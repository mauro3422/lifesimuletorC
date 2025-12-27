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
                      std::vector<StateComponent>& states,
                      const std::vector<int>& rootCache) {
    
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
        std::unordered_map<int, std::vector<int>> subRings;
        subRings.reserve(8); // Optimization Phase 28
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

            // Collaborative Check for this specific ring
            bool ringReady = true;
            for (int idx : subIndices) {
                if (states[idx].dockingProgress < 1.0f) {
                    int rIdx = states[idx].ringIndex;
                    if (rIdx >= 0 && rIdx < (int)offsets.size()) {
                        float dx = (scx + offsets[rIdx].x) - transforms[idx].x;
                        float dy = (scy + offsets[rIdx].y) - transforms[idx].y;
                        if (std::sqrt(dx*dx + dy*dy) > def->completionThreshold) {
                            ringReady = false;
                            break;
                        }
                    }
                }
            }

            // Perform Hard Snap on completion
            if (ringReady) {
                bool firstTimeReady = false;
                for (int idx : subIndices) if (states[idx].dockingProgress < 1.0f) { firstTimeReady = true; break; }

                for (int idx : subIndices) {
                    if (firstTimeReady) {
                        int rIdx = states[idx].ringIndex;
                        if (rIdx >= 0 && rIdx < (int)offsets.size()) {
                            transforms[idx].x = scx + offsets[rIdx].x;
                            transforms[idx].y = scy + offsets[rIdx].y;
                            transforms[idx].z = 0.0f; 
                            transforms[idx].vx = avgVx;
                            transforms[idx].vy = avgVy;
                            transforms[idx].vz = 0.0f;
                        }
                    }
                    states[idx].dockingProgress = 1.0f;
                }
            }

            // Physic Forces & Damping
            for (int idx : subIndices) {
                float currentDamping = (states[idx].dockingProgress < 1.0f) ? def->formationDamping : internalDamping;
                
                float relVx = transforms[idx].vx - avgVx;
                float relVy = transforms[idx].vy - avgVy;

                if (states[idx].dockingProgress < 1.0f) {
                    int rIdx = states[idx].ringIndex;
                    if (rIdx >= 0 && rIdx < (int)offsets.size()) {
                        float targetX = scx + offsets[rIdx].x;
                        float targetY = scy + offsets[rIdx].y;
                        float dx = targetX - transforms[idx].x;
                        float dy = targetY - transforms[idx].y;

                        float pullForce = def->formationSpeed * 80.0f; 
                        relVx += dx * pullForce * dt;
                        relVy += dy * pullForce * dt;

                        float relSpeedSq = relVx*relVx + relVy*relVy;
                        float maxRelSpeed = def->maxFormationSpeed;
                        if (relSpeedSq > maxRelSpeed * maxRelSpeed) {
                            float scale = maxRelSpeed / std::sqrt(relSpeedSq);
                            relVx *= scale;
                            relVy *= scale;
                        }
                    }
                    states[idx].dockingProgress += dt * (def->formationSpeed * 0.2f);
                }

                transforms[idx].vx = (avgVx * globalDriftDamping) + (relVx * currentDamping);
                transforms[idx].vy = (avgVy * globalDriftDamping) + (relVy * currentDamping);
                
                transforms[idx].vz -= transforms[idx].z * 20.0f * dt;
                transforms[idx].vz *= 0.5f;
            }
        }
    }
}

void applyFoldingAndAffinity(float dt,
                            std::vector<TransformComponent>& transforms,
                            const std::vector<AtomComponent>& atoms,
                            std::vector<StateComponent>& states,
                            EnvironmentManager& environment,
                            const std::vector<int>& rootCache) {
    
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
            
            if (d2 > 30.0f*30.0f && d2 < 150.0f*150.0f) {
                float dist = std::sqrt(d2);
                int root1 = rootCache[c1];
                int root2 = rootCache[c2];
                float affinityStrength = (root1 != root2) ? 15.0f : 10.0f;
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
            int root1 = rootCache[t1];
            int root2 = rootCache[t2];
            if (root1 != root2) continue; 
            
            float dx = transforms[t2].x - transforms[t1].x;
            float dy = transforms[t2].y - transforms[t1].y;
            float dz = transforms[t2].z - transforms[t1].z;
            float distSq = dx*dx + dy*dy + dz*dz;
            
            if (distSq > 20.0f*20.0f && distSq < 300.0f*300.0f) {
                float dist = std::sqrt(distSq);
                float foldingStrength = 18.0f;
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
