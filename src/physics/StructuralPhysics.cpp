#include "StructuralPhysics.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../chemistry/StructureDefinition.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "../world/EnvironmentManager.hpp"
#include <unordered_map>
#include <map>
#include <cmath>

namespace StructuralPhysics {

void applyRingDynamics(float dt, 
                      std::vector<TransformComponent>& transforms,
                      const std::vector<AtomComponent>& atoms,
                      std::vector<StateComponent>& states) {
    
    // Phase 28: Small optimization, stack.reserve
    static std::vector<int> stack;
    stack.clear();
    stack.reserve(64);

    std::vector<bool> processed(transforms.size(), false);
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (!states[i].isInRing || processed[i]) continue;

        // 1. Collect all atoms in this specific ring structure (connected component)
        std::vector<int> ringIndices;
        stack.push_back(i);
        processed[i] = true;
        
        while(!stack.empty()){
            int curr = stack.back(); stack.pop_back();
            ringIndices.push_back(curr);
            
            // Check connected atoms (Parent)
            int p = states[curr].parentEntityId;
            if (p != -1 && states[p].isInRing && !processed[p]) {
                processed[p] = true; stack.push_back(p);
            }
            // Check children
            for(int k=0; k < (int)transforms.size(); k++) {
                if (states[k].parentEntityId == curr && states[k].isInRing && !processed[k]) {
                    processed[k] = true; stack.push_back(k);
                }
            }
            // Check Cycle Bond
            int c = states[curr].cycleBondId;
            if (c != -1 && states[c].isInRing && !processed[c]) {
                processed[c] = true; stack.push_back(c);
            }
        }

        // 2. Calculate average DRIFT velocity for the entire molecule
        float avgVx = 0, avgVy = 0;
        for (int idx : ringIndices) {
            avgVx += transforms[idx].vx;
            avgVy += transforms[idx].vy;
        }
        avgVx /= ringIndices.size();
        avgVy /= ringIndices.size();

        // 3. Sub-grouping for specific Ring logic (using ringInstanceId)
        // FIX #15: Remove std::map allocation from hot path
        static std::unordered_map<int, std::vector<int>> subRings;
        subRings.clear(); // Reset without deallocating capacity
        // subRings.reserve(8); // Already reserved from previous runs typically

        for (int idx : ringIndices) {
            if (states[idx].isInRing && states[idx].ringInstanceId != -1) {
                subRings[states[idx].ringInstanceId].push_back(idx);
            }
        }

        // 4. Process each sub-ring independently
        for (auto const& [rId, subIndices] : subRings) {
            int sampleIdx = subIndices[0];
            const StructureDefinition* def = StructureRegistry::getInstance().findMatch(states[sampleIdx].ringSize, atoms[sampleIdx].atomicNumber);
            if (!def) continue;

            // FIX: Skip rings that are fully formed (all dockingProgress == 1.0)
            // This prevents drift from recalculating velocities
            bool ringComplete = true;
            for (int idx : subIndices) {
                if (states[idx].dockingProgress < 1.0f) {
                    ringComplete = false;
                    break;
                }
            }
            if (ringComplete && def->instantFormation) {
                // Completed ring with instant formation - just apply damping, no forces
                for (int idx : subIndices) {
                    transforms[idx].vx *= def->damping;
                    transforms[idx].vy *= def->damping;
                    transforms[idx].vz *= Config::Physics::Z_DAMPING;
                }
                continue;  // Skip the rest of the ring processing
            }

            float internalDamping = def->damping;
            float globalDriftDamping = def->globalDamping;
            std::vector<Vector2> offsets = def->getIdealOffsets(Config::BOND_IDEAL_DIST);

            // Sub-ring centroid
            float scx = 0, scy = 0;
            for (int idx : subIndices) {
                scx += transforms[idx].x;
                scy += transforms[idx].y;
            }
            scx /= subIndices.size();
            scy /= subIndices.size();

            // Check if ALL atoms are CLOSE to their targets (actual distance check)
            const float SNAP_THRESHOLD = 3.0f;  // Snap when within 3 pixels
            bool allClose = true;
            float maxGap = 0;
            for (int idx : subIndices) {
                if (states[idx].dockingProgress < 1.0f) {
                    float dx = states[idx].targetX - transforms[idx].x;
                    float dy = states[idx].targetY - transforms[idx].y;
                    float gap = std::sqrt(dx*dx + dy*dy);
                    if (gap > maxGap) maxGap = gap;
                    if (gap > SNAP_THRESHOLD) {
                        allClose = false;
                    }
                }
            }

            // Perform collective snap when ALL atoms are within threshold
            if (allClose && maxGap > 0) {
                bool needsSnap = false;
                for (int idx : subIndices) {
                    if (states[idx].dockingProgress < 1.0f) {
                        needsSnap = true;
                        break;
                    }
                }

                if (needsSnap) {
                    // LOG BEFORE/AFTER for debugging
                    TraceLog(LOG_INFO, "[SNAP] === Collective snap triggered ===");
                    for (int idx : subIndices) {
                        float dx = states[idx].targetX - transforms[idx].x;
                        float dy = states[idx].targetY - transforms[idx].y;
                        float gap = std::sqrt(dx*dx + dy*dy);
                        TraceLog(LOG_INFO, "[SNAP] Atom %d: (%.1f,%.1f) -> target(%.1f,%.1f) gap=%.1fpx",
                                 idx, transforms[idx].x, transforms[idx].y,
                                 states[idx].targetX, states[idx].targetY, gap);
                    }
                    
                    // SNAP TO STORED TARGETS
                    for (int idx : subIndices) {
                        transforms[idx].x = states[idx].targetX;
                        transforms[idx].y = states[idx].targetY;
                        transforms[idx].z = 0.0f; 
                        transforms[idx].vx = avgVx;
                        transforms[idx].vy = avgVy;
                        transforms[idx].vz = 0.0f;
                        states[idx].dockingProgress = 1.0f;
                    }
                    TraceLog(LOG_INFO, "[SNAP] Snap completed - all atoms at targets");
                    
                    // Phase 45: Freeze structure into super-atom (rigid body mode)
                    static int nextStructureId = 1;
                    int newStructureId = nextStructureId++;
                    for (int idx : subIndices) {
                        states[idx].structureId = newStructureId;
                        states[idx].isFrozen = true;
                    }
                    TraceLog(LOG_INFO, "[STRUCTURE] Frozen ring as structureId=%d with %d atoms", 
                             newStructureId, (int)subIndices.size());
                }
            }

            // Physic Forces & Damping
            for (int idx : subIndices) {
                float currentDamping = (states[idx].dockingProgress < 1.0f) ? def->formationDamping : internalDamping;
                
                float relVx = transforms[idx].vx - avgVx;
                float relVy = transforms[idx].vy - avgVy;

                if (states[idx].dockingProgress < 1.0f) {
                    // Use stored ABSOLUTE target position (set by RingChemistry)
                    float targetX = states[idx].targetX;
                    float targetY = states[idx].targetY;
                    float dx = targetX - transforms[idx].x;
                    float dy = targetY - transforms[idx].y;
                    float distSq = dx*dx + dy*dy;
                    float dist = std::sqrt(distSq);

                    // Strong pull force - increases as atom gets closer (spring-like)
                    float pullForce = def->formationSpeed * Config::Physics::FORMATION_PULL_MULTIPLIER * 3.0f;
                    relVx += dx * pullForce * dt;
                    relVy += dy * pullForce * dt;

                    float relSpeedSq = relVx*relVx + relVy*relVy;
                    float maxRelSpeed = def->maxFormationSpeed;
                    if (relSpeedSq > maxRelSpeed * maxRelSpeed) {
                        float scale = maxRelSpeed / std::sqrt(relSpeedSq);
                        relVx *= scale;
                        relVy *= scale;
                    }
                    
                    // DISTANCE-BASED docking progress (not time-based)
                    // Progress = how close we are to target (0=far, 1=at target)
                    float maxDist = Config::BOND_IDEAL_DIST * 1.5f;  // Start distance reference
                    float progress = 1.0f - std::min(dist / maxDist, 1.0f);
                    states[idx].dockingProgress = std::max(states[idx].dockingProgress, progress);
                    
                    // Cap at 99% - collective snap finishes when ALL atoms are close
                    if (states[idx].dockingProgress > 0.99f) {
                        states[idx].dockingProgress = 0.99f;
                    }
                }

                transforms[idx].vx = (avgVx * globalDriftDamping) + (relVx * currentDamping);
                transforms[idx].vy = (avgVy * globalDriftDamping) + (relVy * currentDamping);
                
                transforms[idx].vz -= transforms[idx].z * Config::Physics::Z_FLATTEN_STRENGTH * dt;
                transforms[idx].vz *= Config::Physics::Z_DAMPING;
            }
        }
    }
}

void applyFoldingAndAffinity(float dt,
                            std::vector<TransformComponent>& transforms,
                            const std::vector<AtomComponent>& atoms,
                            std::vector<StateComponent>& states,
                            EnvironmentManager& environment) {
    
    // --- CARBON AFFINITY ---
    std::vector<int> seekingCarbons;
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (states[i].isInRing) continue; 
        if (atoms[i].atomicNumber != 6) continue; 
        
        float rangeMultiplier = environment.getBondRangeMultiplier({transforms[i].x, transforms[i].y});
        if (rangeMultiplier < 1.2f) continue;
        
        int bondCount = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        if (bondCount < 4) seekingCarbons.push_back(i);
    }
    
    for (size_t a = 0; a < seekingCarbons.size(); a++) {
        for (size_t b = a + 1; b < seekingCarbons.size(); b++) {
            int c1 = seekingCarbons[a];
            int c2 = seekingCarbons[b];
            
            // Phase 28: Use centralized distSq
            float dx = transforms[c2].x - transforms[c1].x;
            float dy = transforms[c2].y - transforms[c1].y;
            float d2 = dx*dx + dy*dy;
            
            if (d2 > Config::Physics::CARBON_AFFINITY_MIN_DIST * Config::Physics::CARBON_AFFINITY_MIN_DIST && 
                d2 < Config::Physics::CARBON_AFFINITY_MAX_DIST * Config::Physics::CARBON_AFFINITY_MAX_DIST) {
                float dist = std::sqrt(d2);
                int root1 = states[c1].moleculeId;
                int root2 = states[c2].moleculeId;
                float affinityStrength = (root1 != root2) ? Config::Physics::CARBON_AFFINITY_STRENGTH_EXTERNAL : Config::Physics::CARBON_AFFINITY_STRENGTH_INTERNAL;
                float nx = dx / dist;
                float ny = dy / dist;
                
                transforms[c1].vx += nx * affinityStrength * dt;
                transforms[c1].vy += ny * affinityStrength * dt;
                transforms[c2].vx -= nx * affinityStrength * dt;
                transforms[c2].vy -= ny * affinityStrength * dt;
            }
        }
    }
    
    // --- RING CLOSING (FOLDING) ---
    std::vector<int> terminals;
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (states[i].isInRing) continue; 
        float rangeMultiplier = environment.getBondRangeMultiplier({transforms[i].x, transforms[i].y});
        if (rangeMultiplier < 1.2f) continue;
        int bondCount = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        if (bondCount == 1) terminals.push_back(i);
    }
    
    for (size_t a = 0; a < terminals.size(); a++) {
        for (size_t b = a + 1; b < terminals.size(); b++) {
            int t1 = terminals[a];
            int t2 = terminals[b];
            int root1 = states[t1].moleculeId;
            int root2 = states[t2].moleculeId;
            if (root1 != root2) continue; 
            
            float dx = transforms[t2].x - transforms[t1].x;
            float dy = transforms[t2].y - transforms[t1].y;
            float dz = transforms[t2].z - transforms[t1].z;
            float distSq = dx*dx + dy*dy + dz*dz;
            
            if (distSq > Config::Physics::RING_FOLDING_MIN_DIST * Config::Physics::RING_FOLDING_MIN_DIST && 
                distSq < Config::Physics::RING_FOLDING_MAX_DIST * Config::Physics::RING_FOLDING_MAX_DIST) {
                float dist = std::sqrt(distSq);
                float foldingStrength = Config::Physics::RING_FOLDING_STRENGTH;
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
                
                transforms[t1].vx += nx * foldingStrength * dt;
                transforms[t1].vy += ny * foldingStrength * dt;
                transforms[t1].vz += nz * foldingStrength * dt;
                transforms[t2].vx -= nx * foldingStrength * dt;
                transforms[t2].vy -= ny * foldingStrength * dt;
                transforms[t2].vz -= nz * foldingStrength * dt;
            }
        }
    }
}

} // namespace
