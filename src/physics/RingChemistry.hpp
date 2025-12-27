#ifndef RING_CHEMISTRY_HPP
#define RING_CHEMISTRY_HPP

#include <vector>
#include <algorithm>
#include "raylib.h"
#include "../ecs/components.hpp"
#include "../core/MathUtils.hpp"
#include "MolecularHierarchy.hpp"

/**
 * RingChemistry (Phase 30)
 * Handles cycle detection and ring formation logic.
 */
class RingChemistry {
public:
    static BondingCore::BondError tryCycleBond(int i, int j, 
                                             std::vector<StateComponent>& states, 
                                             std::vector<AtomComponent>& atoms, 
                                             std::vector<TransformComponent>& transforms) {
        if (i < 0 || j < 0 || i == j) return BondingCore::INTERNAL_ERROR;
        if (states[i].cycleBondId != -1 || states[j].cycleBondId != -1) return BondingCore::ALREADY_BONDED;

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
            return BondingCore::INTERNAL_ERROR;
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

        if (lca == -1) return BondingCore::INTERNAL_ERROR; // Different molecules? Should be filtered by caller.

        int ringSize = distI + distJ + 1;
        
        // PHYSICAL LINK
        states[i].cycleBondId = j;
        states[j].cycleBondId = i;

        // STRUCTURAL TAGGING
        int ringId = i * 1000 + j; // Temporary unique ID
        std::vector<int> ringMembers;
        
        // Trace up from I to LCA
        for (int k = 0; k <= distI; k++) ringMembers.push_back(chainFromI[k]);
        // Trace up from J to LCA (excluding LCA as it's already added)
        for (int k = 0; k < distJ; k++) ringMembers.push_back(chainFromJ[k]);

        for (int idx : ringMembers) {
            states[idx].isInRing = true;
            states[idx].ringSize = ringSize;
            states[idx].ringInstanceId = ringId;
        }

        // --- VISUAL FORMATION (Square Hard-Snap) ---
        if (ringSize == 4) {
             float scx = 0, scy = 0;
             for (int idx : ringMembers) { scx += transforms[idx].x; scy += transforms[idx].y; }
             scx /= 4.0f; scy /= 4.0f;

             // Perfect Square Offsets
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
                 states[idx].ringIndex = k;
             }
        }

        return BondingCore::SUCCESS;
    }
};

#endif // RING_CHEMISTRY_HPP
