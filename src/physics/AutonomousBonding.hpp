#ifndef AUTONOMOUS_BONDING_HPP
#define AUTONOMOUS_BONDING_HPP

#include <vector>
#include "../ecs/components.hpp"
#include "SpatialGrid.hpp"
#include "world/EnvironmentManager.hpp"
#include "BondingCore.hpp"
#include "RingChemistry.hpp"

/**
 * AutonomousBonding (Phase 30)
 * Orchestrates spontaneous molecular evolution.
 */
class AutonomousBonding {
public:
    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         std::vector<AtomComponent>& atoms,
                                         std::vector<TransformComponent>& transforms,
                                         const SpatialGrid& grid,
                                         const std::vector<int>& rootCache,
                                         EnvironmentManager* env = nullptr,
                                         int tractedRoot = -1) {
        
        for (int i = 0; i < (int)states.size(); i++) {
            // EARLY EXIT: prioritize one bond per atom per tick
            if (states[i].justBonded) continue;
            if (states[i].isLocked() && states[i].isInRing) continue;

            // Skip atoms being dragged by tractor
            if (tractedRoot != -1 && (rootCache[i] == tractedRoot)) continue;

            std::vector<int> neighbors = grid.getNearby({transforms[i].x, transforms[i].y}, Config::BOND_AUTO_RANGE * 1.5f);

            for (int j : neighbors) {
                if (j <= i || states[j].justBonded) continue;
                if (j >= (int)states.size()) continue;

                float dx = transforms[i].x - transforms[j].x;
                float dy = transforms[i].y - transforms[j].y;
                float dz = transforms[i].z - transforms[j].z;
                float distSq = dx*dx + dy*dy + dz*dz;

                float rangeMultiplier = 1.0f;
                if (env) rangeMultiplier = env->getBondRangeMultiplier({transforms[i].x, transforms[i].y});
                float currentRange = Config::BOND_AUTO_RANGE * rangeMultiplier;
                
                if (distSq < currentRange * currentRange) {
                    int rootI = rootCache[i];
                    int rootJ = rootCache[j];
                    bool isSameMolecule = (rootI == rootJ);

                    if (isSameMolecule) {
                        // INTERNAL RING CLOSING LOGIC
                        if (states[i].cycleBondId == -1 && states[j].cycleBondId == -1) {
                            int hops = MathUtils::getHierarchyDistance(i, j, states);
                            if (hops >= 3 && hops <= 6) {
                                if (RingChemistry::tryCycleBond(i, j, states, atoms, transforms) == BondingCore::SUCCESS) {
                                    states[i].justBonded = true;
                                    states[j].justBonded = true;
                                    break; 
                                }
                            }
                        }
                    } else {
                        // INTER-MOLECULAR BONDING
                        if (rootI != 0 && rootJ != 0 && !states[rootI].isShielded && !states[rootJ].isShielded) { 
                            if (BondingCore::tryBond(i, j, states, atoms, transforms) == BondingCore::SUCCESS) {
                                states[i].justBonded = true;
                                states[j].justBonded = true;
                                break; 
                            }
                        }
                    }
                }
            }
        }
    }
};

#endif // AUTONOMOUS_BONDING_HPP
