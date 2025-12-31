#ifndef RING_CHEMISTRY_HPP
#define RING_CHEMISTRY_HPP

#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>
#include "raylib.h"
#include "../ecs/components.hpp"
#include "../core/MathUtils.hpp"
#include "../core/Config.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../chemistry/StructureDefinition.hpp"
#include "MolecularHierarchy.hpp"
#include "BondingTypes.hpp"
// BondingCore include might still be needed for logic, but for types we use BondingTypes

/**
 * RingChemistry (Phase 30)
 * Handles cycle detection and ring formation logic.
 */
class RingChemistry {
public:
    static BondError tryCycleBond(int i, int j, 
                                             std::vector<StateComponent>& states, 
                                             std::vector<AtomComponent>& atoms, 
                                             std::vector<TransformComponent>& transforms) {
        if (i < 0 || j < 0 || i == j) return BondError::INTERNAL_ERROR;
        
        // BUG FIX: Allow atoms in a ring to participate in NEW cycle bonds for ladder formation,
        // but they must not already have a cycle bond themselves.
        if (states[i].cycleBondId != -1 || states[j].cycleBondId != -1) return BondError::ALREADY_BONDED;
        
        // --- PATH TRACING (Cycle Validation) ---
        std::vector<int> chainFromI;
        int currI = i;
        int safetyI = 0;
        const int MAX_DEPTH = 100;

        while (currI != -1 && safetyI < MAX_DEPTH) {
            chainFromI.push_back(currI);
            currI = states[currI].parentEntityId;
            safetyI++;
        }
        if (safetyI >= MAX_DEPTH) {
            TraceLog(LOG_ERROR, "[BOND] Infinite loop detected in hierarchy at atom %d", i);
            return BondError::INTERNAL_ERROR;
        }

        std::vector<int> chainFromJ;
        int currJ = j;
        int safetyJ = 0;
        while (currJ != -1 && safetyJ < MAX_DEPTH) {
            chainFromJ.push_back(currJ);
            currJ = states[currJ].parentEntityId;
            safetyJ++;
        }

        // Find Least Common Ancestor (LCA)
        int lca = -1;
        int distI = -1, distJ = -1;
        for (int idxI = 0; idxI < (int)chainFromI.size(); idxI++) {
            for (int idxJ = 0; idxJ < (int)chainFromJ.size(); idxJ++) {
                if (chainFromI[idxI] == chainFromJ[idxJ]) {
                    lca = chainFromI[idxI];
                    distI = idxI;
                    distJ = idxJ;
                    goto found_lca;
                }
            }
        }
        found_lca:

        if (lca == -1) return BondError::INTERNAL_ERROR; // Different molecules? Should be filtered by caller.

        int ringSize = distI + distJ + 1;
        
        // BUG FIX: Reject cycles smaller than 4 atoms (triangles are chemically unstable and shouldn't form membranes)
        if (ringSize < 4) {
            TraceLog(LOG_WARNING, "[RING] Rejected cycle of size %d (minimum is 4 for stable ring)", ringSize);
            return BondError::RING_TOO_SMALL;
        }
        
        // PHYSICAL LINK
        states[i].cycleBondId = j;
        states[j].cycleBondId = i;
        
        // Synchronize cluster IDs
        MolecularHierarchy::propagateMoleculeId(i, states);

        // STRUCTURAL TAGGING
        // FIX #3: Ring Instance ID Overflow Protection
        static int nextRingId = 1;
        static constexpr int MAX_RING_ID = 1000000;
        
        int ringId = nextRingId++;
        if (nextRingId >= MAX_RING_ID) {
            nextRingId = 1; // Wrap around safely
        }
        
        // BUG FIX: Build ringMembers in CORRECT ORDER (chain from I to J via LCA)
        // This ensures positions are assigned sequentially around the ring
        std::vector<int> ringMembers;
        
        // Path I -> LCA (in order from I going up)
        for (int k = 0; k <= distI; k++) {
            ringMembers.push_back(chainFromI[k]);
        }
        // Path LCA -> J (in reverse order, excluding LCA which is already added)
        for (int k = distJ - 1; k >= 0; k--) {
            ringMembers.push_back(chainFromJ[k]);
        }

        // Check if any atom was ALREADY in a VALID ring (for fusion detection)
        // Only count as "was in ring" if they have a different ringInstanceId AND a valid cycleBond
        bool anyWasInRing = false;
        for (int atomId : ringMembers) {
            // Only consider it a "previous ring" if they have a valid cycle bond
            // AND a different ring instance (not -1 or 0)
            if (states[atomId].isInRing && 
                states[atomId].ringInstanceId > 0 && 
                states[atomId].cycleBondId >= 0 &&
                states[atomId].cycleBondId < (int)states.size() &&
                states[states[atomId].cycleBondId].cycleBondId == atomId) {  // Mutual cycle bond
                anyWasInRing = true;
                break;
            }
        }

        // Calculate centroid for proper ringIndex assignment
        float cx = 0, cy = 0;
        for (int atomId : ringMembers) {
            cx += transforms[atomId].x;
            cy += transforms[atomId].y;
        }
        cx /= (float)ringSize;
        cy /= (float)ringSize;
        
        // Sort atoms by angle around centroid for correct geometry
        std::vector<std::pair<float, int>> angleAtom;
        for (int atomId : ringMembers) {
            float dx = transforms[atomId].x - cx;
            float dy = transforms[atomId].y - cy;
            float angle = std::atan2(dy, dx);
            angleAtom.push_back({angle, atomId});
        }
        std::sort(angleAtom.begin(), angleAtom.end());
        
        // Assign ringIndex based on sorted angular position
        for (int idx = 0; idx < (int)angleAtom.size(); idx++) {
            int atomId = angleAtom[idx].second;
            states[atomId].isInRing = true;
            states[atomId].ringSize = ringSize;
            states[atomId].ringInstanceId = ringId;
            states[atomId].ringIndex = idx;  // Now based on angular order
        }

        // --- VISUAL FORMATION (Generalized Polygon Hard-Snap) ---
        // Only trigger hard-snap if these atoms were NOT in another ring already
        if (ringSize >= 4 && ringSize <= 8 && !anyWasInRing) {
            // Look up structure definition for this polygon size
            const StructureDefinition* def = StructureRegistry::getInstance()
                .findMatch(ringSize, atoms[ringMembers[0]].atomicNumber);
            
            if (def) {
                // Use already-calculated centroid (cx, cy) from angular sorting above
                // Use rotation from structure definition (configurable per structure)
                float fixedAngle = def->rotationOffset;  // From structures.json
                float angleStep = (2.0f * 3.1415926535f) / ringSize;
                float radius = Config::BOND_IDEAL_DIST / (2.0f * std::sin(3.1415926535f / ringSize));
                
                std::vector<Vector2> offsets;
                for (int i = 0; i < ringSize; i++) {
                    float currentAngle = fixedAngle + i * angleStep;
                    offsets.push_back({
                        std::cos(currentAngle) * radius,
                        std::sin(currentAngle) * radius
                    });
                }
                
                // Hard snap ONLY if instantFormation is enabled
                if (def->instantFormation) {
                    // Find best starting offset for first atom, then assign consecutively
                    // This preserves ring topology (adjacent atoms get adjacent offsets)
                    int firstAtom = ringMembers[0];
                    float firstX = transforms[firstAtom].x - cx;
                    float firstY = transforms[firstAtom].y - cy;
                    
                    // Find which offset is closest to first atom's position
                    int startK = 0;
                    float bestDist = 1e9f;
                    for (int k = 0; k < ringSize; k++) {
                        float dx = offsets[k].x - firstX;
                        float dy = offsets[k].y - firstY;
                        float dist = dx*dx + dy*dy;
                        if (dist < bestDist) {
                            bestDist = dist;
                            startK = k;
                        }
                    }
                    
                    // Assign consecutive offsets starting from best match
                    for (int i = 0; i < ringSize; i++) {
                        int atomId = ringMembers[i];
                        int k = (startK + i) % ringSize;
                        
                        states[atomId].ringIndex = k;
                        float tgtX = cx + offsets[k].x;
                        float tgtY = cy + offsets[k].y;
                        transforms[atomId].x = tgtX;
                        transforms[atomId].y = tgtY;
                        transforms[atomId].z = 0.0f;
                        transforms[atomId].vx = transforms[atomId].vy = transforms[atomId].vz = 0.0f;
                        states[atomId].dockingProgress = 1.0f;
                        states[atomId].targetX = tgtX;
                        states[atomId].targetY = tgtY;
                    }
                } else {
                    // Gradual animation: same topology-preserving logic
                    int firstAtom = ringMembers[0];
                    float firstX = transforms[firstAtom].x - cx;
                    float firstY = transforms[firstAtom].y - cy;
                    
                    int startK = 0;
                    float bestDist = 1e9f;
                    for (int k = 0; k < ringSize; k++) {
                        float dx = offsets[k].x - firstX;
                        float dy = offsets[k].y - firstY;
                        float dist = dx*dx + dy*dy;
                        if (dist < bestDist) {
                            bestDist = dist;
                            startK = k;
                        }
                    }
                    
                    for (int i = 0; i < ringSize; i++) {
                        int atomId = ringMembers[i];
                        int k = (startK + i) % ringSize;
                        
                        states[atomId].ringIndex = k;
                        states[atomId].dockingProgress = 0.0f;
                        states[atomId].targetX = cx + offsets[k].x;
                        states[atomId].targetY = cy + offsets[k].y;
                    }
                }
                
                TraceLog(LOG_INFO, "[RING] Formed %d-ring at (%.0f, %.0f) with %d atoms%s",
                         ringSize, cx, cy, ringSize, 
                         def->instantFormation ? "" : " (gradual animation)");
            } else {
                // Fallback for rings without structure definition
                TraceLog(LOG_WARNING, "[RING] No structure definition for ring size %d. Skipping hard-snap.", ringSize);
            }
        } else if (ringSize >= 4 && anyWasInRing) {
            TraceLog(LOG_INFO, "[RING] Formed %d-ring (fused). Skipping hard-snap for stability.", ringSize);
        }

        return BondError::SUCCESS;
    }

    /**
     * Centralized Ring Invalidation.
     * When one bond of a ring breaks, the entire ring structure (visuals/flags) must be invalidated.
     * Phase 43 FIX: Also handles edge cases where ringId is invalid.
     */
    static void invalidateRing(int ringId, std::vector<StateComponent>& states) {
        bool found = false;
        
        // If ringId is valid, invalidate by ringId
        if (ringId > 0) {
            for (size_t i = 0; i < states.size(); i++) {
                if (states[i].ringInstanceId == ringId) {
                    states[i].isInRing = false;
                    states[i].ringInstanceId = -1;
                    states[i].ringSize = 0;
                    states[i].ringIndex = -1;
                    states[i].dockingProgress = 0.0f;  // Reset to NOT locked
                    
                    // FIX (Phase 42): Clear cycleBondId when ring is invalidated.
                    int partner = states[i].cycleBondId;
                    if (partner != -1 && partner < (int)states.size()) {
                        states[partner].cycleBondId = -1;
                    }
                    states[i].cycleBondId = -1;
                    
                    found = true;
                }
            }
        }
        
        if (found) {
            TraceLog(LOG_INFO, "[RING] Invalidated entire ring instance metadata: %d", ringId);
        }
    }
    
    /**
     * Force-clears ALL ring flags from a specific atom.
     * Used when isolating an atom via tractor beam.
     */
    static void clearRingFlags(int atomId, std::vector<StateComponent>& states) {
        if (atomId < 0 || atomId >= (int)states.size()) return;
        
        // Clear partner's cycleBondId first
        int partner = states[atomId].cycleBondId;
        if (partner != -1 && partner < (int)states.size()) {
            states[partner].cycleBondId = -1;
        }
        
        // Clear this atom's ring flags
        states[atomId].isInRing = false;
        states[atomId].ringInstanceId = -1;
        states[atomId].ringSize = 0;
        states[atomId].ringIndex = -1;
        states[atomId].cycleBondId = -1;
        states[atomId].dockingProgress = 1.0f;
    }
};

#endif // RING_CHEMISTRY_HPP
