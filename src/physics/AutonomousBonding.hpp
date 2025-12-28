#ifndef AUTONOMOUS_BONDING_HPP
#define AUTONOMOUS_BONDING_HPP

#include <vector>
#include <map> 
#include <iterator>
#include <algorithm>
#include "../ecs/components.hpp"
#include "SpatialGrid.hpp"
#include "world/EnvironmentManager.hpp"
#include "BondingCore.hpp"
#include "RingChemistry.hpp"
#include "BondingTypes.hpp"

/**
 * AutonomousBonding (Phase 30)
 * Orchestrates spontaneous molecular evolution.
 */
class AutonomousBonding {
public:
// ... (Existing code)
    
    // Helper: Calculate Ring Centroid
    static Vector3 getCentroid(const std::vector<int>& members, const std::vector<TransformComponent>& transforms) {
        Vector3 sum = {0, 0, 0};
        for (int id : members) {
            sum.x += transforms[id].x;
            sum.y += transforms[id].y;
            sum.z += transforms[id].z;
        }
        float inv = 1.0f / members.size();
        return {sum.x * inv, sum.y * inv, sum.z * inv};
    }

    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         std::vector<AtomComponent>& atoms,
                                         std::vector<TransformComponent>& transforms,
                                         const SpatialGrid& grid,
                                         const std::vector<int>& rootCache,
                                         EnvironmentManager* env = nullptr,
                                         int tractedRoot = -1) {
        
        // 1. MACRO-ALIGNMENT (Phase 18: Structure Magnetism)
        // Group atoms by ringInstanceId to treat them as Rigid Bodies
        std::map<int, std::vector<int>> rings;
        for (size_t i = 1; i < states.size(); i++) { // Skip player
            if (states[i].isInRing && states[i].ringInstanceId != -1) {
                // Check rootCache to group full molecules is done later
                rings[states[i].ringInstanceId].push_back((int)i);
            }
        }

        // Process Pairs of Rings
        // Note: O(R^2) is fine for small N. For large N, use Grid.
        for (auto it1 = rings.begin(); it1 != rings.end(); ++it1) {
            for (auto it2 = std::next(it1); it2 != rings.end(); ++it2) {
                int ringIdA = it1->first;
                int ringIdB = it2->first;
                
                // Skip if same molecule (already bonded)
                int repA = it1->second[0];
                int repB = it2->second[0];
                if (rootCache[repA] == rootCache[repB]) continue;

                // Calculate Centroids
                Vector3 cA = getCentroid(it1->second, transforms);
                Vector3 cB = getCentroid(it2->second, transforms);
                
                float distSq = MathUtils::distSq(cA.x, cA.y, cB.x, cB.y);
                
                // MACRO-RANGE: 100 units (larger than atomic bond range)
                if (distSq < 100.0f * 100.0f) {
                    // DOCKING PORTS LOGIC
                    // Instead of complex orientation, attract Closest Atoms to simulate Port-Magnetism
                    int bestA = -1, bestB = -1;
                    float minAtomDistSq = 999999.0f;

                    for (int a : it1->second) {
                        for (int b : it2->second) {
                            float d = MathUtils::distSq(transforms[a].x, transforms[a].y, transforms[b].x, transforms[b].y);
                            if (d < minAtomDistSq) {
                                minAtomDistSq = d;
                                bestA = a;
                                bestB = b;
                            }
                        }
                    }

                    // Apply Magnetic Force to Closest Point (Torque + Attraction)
                    if (bestA != -1 && bestB != -1) {
                        float dist = std::sqrt(minAtomDistSq);
                        if (dist > Config::BOND_IDEAL_DIST) {
                            float force = 50.0f * (1.0f - dist / 100.0f); // Falloff
                            Vector3 dir = MathUtils::safeNormalize({transforms[bestB].x - transforms[bestA].x, 
                                                                    transforms[bestB].y - transforms[bestA].y, 0});
                            
                            // Move atoms (Rigid body propagation handle by physics engine stiffness)
                            transforms[bestA].vx += dir.x * force * 0.016f;
                            transforms[bestA].vy += dir.y * force * 0.016f;
                            transforms[bestB].vx -= dir.x * force * 0.016f;
                            transforms[bestB].vy -= dir.y * force * 0.016f;
                            
                            // Debug Line
                            // TraceLog(LOG_INFO, "Magnetic Pull: Ring %d -> Ring %d", ringIdA, ringIdB);
                        }
                    }
                }
            }
        }

        // 2. MICRO-BONDING (Existing Logic)
        for (int i = 0; i < (int)states.size(); i++) {
            // ... (rest of function)
            // EARLY EXIT: prioritize one bond per atom per tick
            if (states[i].justBonded) continue;
            if (states[i].isLocked() && states[i].isInRing) continue;

            // Skip atoms being dragged by tractor
            if (tractedRoot != -1 && (rootCache[i] == tractedRoot)) continue;

            std::vector<int> neighbors = grid.getNearby({transforms[i].x, transforms[i].y}, Config::BOND_AUTO_RANGE * 1.5f);

            // CRITICAL FIX: Sort neighbors by distance to prevent "Cross-Threading" (Tangling)
            // Example: In a square, diagonal is further than edge. We MUST bond edge first.
            std::sort(neighbors.begin(), neighbors.end(), [&](int a, int b) {
                float da = MathUtils::distSq(transforms[i].x, transforms[i].y, transforms[a].x, transforms[a].y);
                float db = MathUtils::distSq(transforms[i].x, transforms[i].y, transforms[b].x, transforms[b].y);
                return da < db;
            });

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
                                if ((BondError)RingChemistry::tryCycleBond(i, j, states, atoms, transforms) == BondError::SUCCESS) {
                                    states[i].justBonded = true;
                                    states[j].justBonded = true;
                                    break; 
                                }
                            }
                        }
                    } else {
                        // INTER-MOLECULAR BONDING
                        if (rootI != 0 && rootJ != 0 && !states[rootI].isShielded && !states[rootJ].isShielded) { 
                            if ((BondError)BondingCore::tryBond(i, j, states, atoms, transforms) == BondError::SUCCESS) {
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
