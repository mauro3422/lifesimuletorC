#ifndef RING_CHEMISTRY_HPP
#define RING_CHEMISTRY_HPP

#include <vector>
#include <algorithm>
#include "raylib.h"
#include "../ecs/components.hpp"
#include "../core/MathUtils.hpp"
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

        for (int idx = 0; idx < (int)ringMembers.size(); idx++) {
            int atomId = ringMembers[idx];
            states[atomId].isInRing = true;
            states[atomId].ringSize = ringSize;
            states[atomId].ringInstanceId = ringId;
            states[atomId].ringIndex = idx;  // Assign index here for all ring sizes
        }

        // --- VISUAL FORMATION (Square Hard-Snap) ---
        if (ringSize == 4) {
             float scx = 0, scy = 0;
             for (int idx : ringMembers) { scx += transforms[idx].x; scy += transforms[idx].y; }
             scx /= 4.0f; scy /= 4.0f;

             // Perfect Square Offsets (clockwise order: top-left, top-right, bottom-right, bottom-left)
             float h = Config::BOND_IDEAL_DIST * 0.5f;
             float ox[4] = {-h, h, h, -h};
             float oy[4] = {-h, -h, h, h};

             for (int k = 0; k < 4; k++) {
                 int idx = ringMembers[k];
                 transforms[idx].x = scx + ox[k];
                 transforms[idx].y = scy + oy[k];
                 transforms[idx].z = 0.0f;
                 transforms[idx].vx = transforms[idx].vy = transforms[idx].vz = 0.0f;
                 states[idx].dockingProgress = 1.0f; 
             }
             
             TraceLog(LOG_INFO, "[RING] Formed 4-ring (square) at (%.0f, %.0f) with atoms %d-%d-%d-%d",
                      scx, scy, ringMembers[0], ringMembers[1], ringMembers[2], ringMembers[3]);
        }

        return BondError::SUCCESS;
    }
};

#endif // RING_CHEMISTRY_HPP
