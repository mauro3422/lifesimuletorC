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

        // 1.5 ATOM-TO-RING ALIGNMENT (Phase 32: Square Stacking Preparation)
        // When an atom is connected to a ring, apply alignment force to prepare for next square formation
        for (int i = 1; i < (int)states.size(); i++) {
            // Skip atoms dragged by tractor or already in a ring
            if (tractedRoot != -1 && rootCache[i] == tractedRoot) continue;
            if (states[i].isInRing) continue;
            if (!states[i].isClustered) continue;
            
            // Check if this atom's parent is in a ring (connected to a square)
            int parent = states[i].parentEntityId;
            if (parent == -1 || parent >= (int)states.size()) continue;
            if (!states[parent].isInRing) continue;
            
            // This atom is connected to a ring! Apply alignment force
            // Calculate ideal position: perpendicular to the parent's ring, away from centroid
            int ringId = states[parent].ringInstanceId;
            if (ringId == -1) continue;
            
            // Find ring members
            std::vector<int> ringMembers;
            for (int k = 0; k < (int)states.size(); k++) {
                if (states[k].ringInstanceId == ringId) {
                    ringMembers.push_back(k);
                }
            }
            if (ringMembers.size() < 4) continue;  // Minimum ring size
            
            // Calculate ring centroid
            float cx = 0, cy = 0;
            for (int m : ringMembers) {
                cx += transforms[m].x;
                cy += transforms[m].y;
            }
            cx /= ringMembers.size();
            cy /= ringMembers.size();
            
            // 1.5.1 LADDER-SLOT ALIGNMENT (Phase 32 Optimization)
            // For 4-rings (squares), align to the extensions of existing bonds (90-deg)
            // instead of a diagonal outward vector.
            bool alignedToSlot = false;
            if (ringMembers.size() >= 4) {  // Works for squares, hexagons, etc.
                // Find ring neighbors of parent
                std::vector<int> neighbors;
                for (int m : ringMembers) {
                    if (m == parent) continue;
                    // Are they bonded?
                    if (states[m].parentEntityId == parent || states[parent].parentEntityId == m || 
                        states[m].cycleBondId == parent || states[parent].cycleBondId == m) {
                        neighbors.push_back(m);
                    }
                }
                
                if (neighbors.size() >= 2) {
                    // Extensions of the two bonds connected to parent in the ring
                    float bestDx = 0, bestDy = 0;
                    float minSlotDistSq = 1e10f;
                    
                    for (int n : neighbors) {
                        float ex = transforms[parent].x - transforms[n].x;
                        float ey = transforms[parent].y - transforms[n].y;
                        float eLen = std::sqrt(ex*ex + ey*ey);
                        if (eLen < 1.0f) continue;
                        ex /= eLen; ey /= eLen;
                        
                        float slotX = transforms[parent].x + ex * Config::BOND_IDEAL_DIST;
                        float slotY = transforms[parent].y + ey * Config::BOND_IDEAL_DIST;
                        
                        float dSq = (slotX - transforms[i].x)*(slotX - transforms[i].x) + 
                                    (slotY - transforms[i].y)*(slotY - transforms[i].y);
                                    
                        if (dSq < minSlotDistSq) {
                            minSlotDistSq = dSq;
                            bestDx = slotX - transforms[i].x;
                            bestDy = slotY - transforms[i].y;
                        }
                    }
                    
                    if (minSlotDistSq < 1e10f) {
                        float dist = std::sqrt(minSlotDistSq);
                        if (dist > 0.1f) {
                            float alignForce = 150.0f; // Snappy axis alignment
                            transforms[i].vx += (bestDx / dist) * alignForce * 0.016f;
                            transforms[i].vy += (bestDy / dist) * alignForce * 0.016f;
                            
                            // Damping for stability
                            if (dist < 15.0f) {
                                transforms[i].vx *= 0.85f;
                                transforms[i].vy *= 0.85f;
                            }
                        }
                        alignedToSlot = true;
                    }
                }
            }
            
            if (alignedToSlot) continue; 

            // Fallback: Calculate direction from centroid through parent (outward direction)
            float dirX = transforms[parent].x - cx;
            float dirY = transforms[parent].y - cy;
            float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
            if (dirLen < 1.0f) continue;
            dirX /= dirLen;
            dirY /= dirLen;
            
            // Target position: parent position + outward direction * bondDistance
            float targetX = transforms[parent].x + dirX * Config::BOND_IDEAL_DIST;
            float targetY = transforms[parent].y + dirY * Config::BOND_IDEAL_DIST;
            
            // Apply alignment force toward target position (for non-square rings)
            float dx = targetX - transforms[i].x;
            float dy = targetY - transforms[i].y;
            float distSq = dx * dx + dy * dy;
            
            if (distSq > 1.0f) { // Only apply if not already perfectly aligned
                float dist = std::sqrt(distSq);
                float alignForce = 120.0f; // Increased force for snappier stacking (Phase 32)
                
                // Falloff: ignore if too far (don't pull random atoms)
                if (dist < Config::BOND_AUTO_RANGE * 2.0f) {
                    float t = 1.0f - (dist / (Config::BOND_AUTO_RANGE * 2.0f));
                    float currentForce = alignForce * t;
                    
                    transforms[i].vx += (dx / dist) * currentForce * 0.016f;
                    transforms[i].vy += (dy / dist) * currentForce * 0.016f;
                    
                    // Stability Damping: reduce perpendicular momentum to target
                    if (dist < 10.0f) {
                         transforms[i].vx *= 0.9f;
                         transforms[i].vy *= 0.9f;
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
                        // INTERNAL RING CLOSING LOGIC - Only in zones that allow it (Clay Zone)
                        bool inRingZone = env && env->isInRingFormingZone({transforms[i].x, transforms[i].y});
                        if (inRingZone && states[i].cycleBondId == -1 && states[j].cycleBondId == -1) {
                            int hops = MathUtils::getHierarchyDistance(i, j, states);
                            if (hops >= 3 && hops <= 7) {  // 4-6 atoms can close rings
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
