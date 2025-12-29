#ifndef RING_CHEMISTRY_HPP
#define RING_CHEMISTRY_HPP

#include <vector>
#include <algorithm>
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

        bool anyWasInRing = false;
        for (int atomId : ringMembers) {
            if (states[atomId].isInRing) {
                anyWasInRing = true;
                break;
            }
        }

        for (int idx = 0; idx < (int)ringMembers.size(); idx++) {
            int atomId = ringMembers[idx];
            states[atomId].isInRing = true;
            states[atomId].ringSize = ringSize;
            states[atomId].ringInstanceId = ringId;
            states[atomId].ringIndex = idx;  // Assign index here for all ring sizes
        }

        // --- VISUAL FORMATION (Generalized Polygon Hard-Snap) ---
        // Only trigger hard-snap if these atoms were NOT in another ring already
        if (ringSize >= 4 && ringSize <= 8 && !anyWasInRing) {
            // Look up structure definition for this polygon size
            const StructureDefinition* def = StructureRegistry::getInstance()
                .findMatch(ringSize, atoms[ringMembers[0]].atomicNumber);
            
            if (def) {
                // Calculate centroid
                float scx = 0, scy = 0;
                for (int idx : ringMembers) { 
                    scx += transforms[idx].x; 
                    scy += transforms[idx].y; 
                }
                scx /= (float)ringSize; 
                scy /= (float)ringSize;

                // Get ideal offsets for this polygon
                std::vector<Vector2> offsets = def->getIdealOffsets(Config::BOND_IDEAL_DIST);
                
                // Snap each atom to its ideal position
                for (int k = 0; k < ringSize && k < (int)offsets.size(); k++) {
                    int idx = ringMembers[k];
                    transforms[idx].x = scx + offsets[k].x;
                    transforms[idx].y = scy + offsets[k].y;
                    transforms[idx].z = 0.0f;
                    transforms[idx].vx = transforms[idx].vy = transforms[idx].vz = 0.0f;
                    states[idx].dockingProgress = 1.0f;
                }
                
                TraceLog(LOG_INFO, "[RING] Formed %d-ring at (%.0f, %.0f) with %d atoms",
                         ringSize, scx, scy, ringSize);
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
     */
    static void invalidateRing(int ringId, std::vector<StateComponent>& states) {
        if (ringId <= 0) return;
        
        bool found = false;
        for (size_t i = 0; i < states.size(); i++) {
            if (states[i].ringInstanceId == ringId) {
                states[i].isInRing = false;
                states[i].ringInstanceId = -1;
                states[i].ringSize = 0;
                states[i].cycleBondId = -1;
                found = true;
            }
        }
        if (found) {
            TraceLog(LOG_INFO, "[RING] Invalidated entire ring instance: %d", ringId);
        }
    }
};

#endif // RING_CHEMISTRY_HPP
