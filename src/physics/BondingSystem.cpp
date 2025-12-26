#include "BondingSystem.hpp"
#include "SpatialGrid.hpp"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"
#include "gameplay/MissionManager.hpp"
#include "world/EnvironmentManager.hpp"
#include <cmath>

int BondingSystem::getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
    if (parentId < 0 || parentId >= (int)states.size()) return -1;

    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& element = db.getElement(atoms[parentId].atomicNumber);
    
    // Count existing bonds (Parent + Children)
    int currentBonds = (states[parentId].parentEntityId != -1 ? 1 : 0) + states[parentId].childCount;

    if (currentBonds >= element.maxBonds) return -1;

    // Find first bit not set in occupiedSlots
    for (int i = 0; i < (int)element.bondingSlots.size(); i++) {
        if (!(states[parentId].occupiedSlots & (1 << i))) return i;
    }
    return -1;
}

BondingSystem::BondError BondingSystem::tryBond(int sourceId, int targetId, 
                           std::vector<StateComponent>& states,
                           std::vector<AtomComponent>& atoms,
                           const std::vector<TransformComponent>& transforms,
                           bool forced,
                           float angleMultiplier) {
    if (sourceId < 0 || targetId < 0 || sourceId == targetId) return INTERNAL_ERROR;
    if (states[sourceId].isClustered) return ALREADY_CLUSTERED; 

    // SMART SCANNER: Search for ALL members of the same molecule using dynamic hierarchy
    int molRootId = MathUtils::findMoleculeRoot(targetId, states);
    
    std::vector<int> candidates;
    for (int i = 0; i < (int)states.size(); i++) {
        // Any atom sharing the same root belongs to this molecule
        if (MathUtils::findMoleculeRoot(i, states) == molRootId) {
            candidates.push_back(i);
        }
    }

    int bestHostId = -1;
    int bestSlotIdx = -1;
    float minSourceDist = Config::FLOAT_MAX;
    bool moleculeHasAnyFreeSlot = false;

    for (int hostId : candidates) {
        const TransformComponent& hostTr = transforms[hostId];
        const TransformComponent& sourceTr = transforms[sourceId];
        
        Vector3 relPos = { sourceTr.x - hostTr.x, sourceTr.y - hostTr.y, sourceTr.z - hostTr.z };
        
        // Verify if this host has ANY free slot before evaluating angle
        if (getFirstFreeSlot(hostId, states, atoms) != -1) {
            moleculeHasAnyFreeSlot = true;
        }

        // Forced mode ignores angle during best slot search
        int slotIdx = getBestAvailableSlot(hostId, relPos, states, atoms, forced, angleMultiplier);
        
        // AUTO-ACCOMMODATION: If forced mode and no slot found by angle,
        // attempt to find the first free slot in this host within the molecule.
        if (slotIdx == -1 && forced) {
            slotIdx = getFirstFreeSlot(hostId, states, atoms);
        }

        if (slotIdx != -1) {
            float dist = std::sqrt(relPos.x*relPos.x + relPos.y*relPos.y + relPos.z*relPos.z);
            // Priority: Geographically closest
            if (dist < minSourceDist) {
                minSourceDist = dist;
                bestHostId = hostId;
                bestSlotIdx = slotIdx;
            }
        }
    }

    if (bestHostId != -1) { // 4. PERFORM BOND
        states[sourceId].isClustered = true;
        states[sourceId].isClustered = true;
        states[sourceId].parentEntityId = bestHostId; 
        states[sourceId].parentSlotIndex = bestSlotIdx;
        states[sourceId].moleculeId = molRootId; 
        states[sourceId].dockingProgress = 0.0f; 

        // Update Optimization Fields (O(1))
        states[bestHostId].childCount++;
        states[bestHostId].occupiedSlots |= (1 << bestSlotIdx);
        
        // --- POLARITY CALCULATION (Partial Charge) ---
        float enHost = ChemistryDatabase::getInstance().getElement(atoms[bestHostId].atomicNumber).electronegativity;
        float enSource = ChemistryDatabase::getInstance().getElement(atoms[sourceId].atomicNumber).electronegativity;
        float polarity = (enHost - enSource) * Config::POLARITY_FACTOR; 
        atoms[bestHostId].partialCharge += polarity;
        atoms[sourceId].partialCharge -= polarity;

        // TraceLog(LOG_INFO, "[BOND] GLOBAL SUCCESS: %d -> %d (Molecule %d) Polarity: %.2f", sourceId, bestHostId, molRootId, polarity);
        (void)polarity; // Silence unused warning
        
        // Notify mission system
        MissionManager::getInstance().notifyBondCreated(atoms[sourceId].atomicNumber, atoms[bestHostId].atomicNumber);
        
        return SUCCESS;
    }

    // --- UNIVERSAL SPLICE LOGIC ---
    // If the molecule is saturated but the player forces the bond (Tractor Beam),
    // apply "Bond Interception" rules based on valency.
    if (forced) {
        const Element& sourceElem = ChemistryDatabase::getInstance().getElement(atoms[sourceId].atomicNumber);
        
        // Universal Rule A: Only atoms capable of forming bridges (valency >= 2) can splice.
        if (sourceElem.maxBonds >= 2) {
            int closestHost = -1;
            float minDist = Config::FLOAT_MAX;
            for (int hostId : candidates) {
                float dist = MathUtils::dist(transforms[sourceId].x, transforms[sourceId].y, transforms[hostId].x, transforms[hostId].y);
                if (dist < minDist) {
                    minDist = dist;
                    closestHost = hostId;
                }
            }

            if (closestHost != -1) {
                // Interception Algorithm:
                // Search for any existing connection from the host to insert the new atom in the middle.
                
                int connectionId = -1;
                bool isParentConn = false;

                if (states[closestHost].parentEntityId != -1) {
                    connectionId = states[closestHost].parentEntityId;
                    isParentConn = true;
                } else {
                    // If host is root, look for its first child
                    for (int i = 0; i < (int)states.size(); i++) {
                        if (states[i].isClustered && states[i].parentEntityId == closestHost) {
                            connectionId = i;
                            isParentConn = false;
                            break;
                        }
                    }
                }

                if (connectionId != -1) {
                    // INTEGRITY RULE: The atom acting as a bridge MUST have valency >= 2.
                    // We cannot "push" a hydrogen to become a bridge.
                    
                    if (isParentConn) {
                        // CASE A: Insert between Host and its Parent (Parent -> Source -> Host)
                        // Verify if Source (new bridge) has valency for both.
                        if (sourceElem.maxBonds < 2) return VALENCY_FULL; // Hydrogen cannot be a bridge

                        int oldParentId = states[closestHost].parentEntityId;
                        int oldSlot = states[closestHost].parentSlotIndex;

                        states[closestHost].parentEntityId = sourceId;
                        states[closestHost].parentSlotIndex = 0; // Inserted into bridge's initial arm

                        states[sourceId].isClustered = true;
                        states[sourceId].parentEntityId = oldParentId;
                        states[sourceId].parentSlotIndex = oldSlot;
                        states[sourceId].moleculeId = molRootId;
                        states[sourceId].dockingProgress = 0.0f;
                    } else {
                        // CASE B: Insert between Host and its Child (Host -> Source -> Child)
                        int oldChildId = connectionId;
                        int oldSlot = states[oldChildId].parentSlotIndex;

                        states[oldChildId].parentEntityId = sourceId;
                        states[oldChildId].parentSlotIndex = 0;

                        states[sourceId].isClustered = true;
                        states[sourceId].parentEntityId = closestHost;
                        states[sourceId].parentSlotIndex = oldSlot;
                        states[sourceId].moleculeId = molRootId;
                        states[sourceId].dockingProgress = 0.0f;
                    }
                    
                    TraceLog(LOG_INFO, "[BOND] SPLICE: Atom %d used as bridge for %d", sourceId, molRootId);
                    
                    // --- POLARITY CALCULATION (Partial Charge) ---
                    float enHost = ChemistryDatabase::getInstance().getElement(atoms[closestHost].atomicNumber).electronegativity;
                    float enSource = ChemistryDatabase::getInstance().getElement(atoms[sourceId].atomicNumber).electronegativity;
                    float polarity = (enHost - enSource) * Config::POLARITY_FACTOR; 
                    atoms[closestHost].partialCharge += polarity;
                    atoms[sourceId].partialCharge -= polarity;

                    return SUCCESS;
                }
            }
        }
    }

    return moleculeHasAnyFreeSlot ? ANGLE_INCOMPATIBLE : VALENCY_FULL;
}

int BondingSystem::getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle,
                                   float angleMultiplier) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& element = db.getElement(atoms[parentId].atomicNumber);
    
    float len = MathUtils::length(relativePos.x, relativePos.y, relativePos.z);
    if (len < 0.001f) return -1;
    Vector3 dir = { relativePos.x/len, relativePos.y/len, relativePos.z/len };

    // TOTAL VALENCY VALIDATION (Parent + Children)
    int currentBonds = (states[parentId].parentEntityId != -1 ? 1 : 0) + states[parentId].childCount;
    if (currentBonds >= element.maxBonds) return -1;

    int bestSlot = -1;
    float maxDot = -Config::FLOAT_MAX; 

    for (int i = 0; i < (int)element.bondingSlots.size(); i++) {
        if (states[parentId].occupiedSlots & (1 << i)) continue; // Already occupied

        Vector3 slotDir = element.bondingSlots[i];
        float dot = dir.x*slotDir.x + dir.y*slotDir.y + dir.z*slotDir.z;
        if (dot > maxDot) {
            maxDot = dot;
            bestSlot = i;
        }
    }

    if (bestSlot == -1) return -1; // No valency slots available
    
    // If angle ignored (Forced Mode), return the best available slot
    if (ignoreAngle) return bestSlot;

    // Otherwise, verify geometric threshold (Natural/NPC Mode)
    return (maxDot > (Config::BOND_SNAP_THRESHOLD / angleMultiplier)) ? bestSlot : -1; 
}

void BondingSystem::updateHierarchy(std::vector<TransformComponent>& transforms,
                                   std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();

    for (int i = 0; i < (int)transforms.size(); i++) {
        StateComponent& state = states[i];
        if (state.isClustered && state.parentEntityId != -1) {
            int pId = state.parentEntityId;
            const Element& parentElement = db.getElement(atoms[pId].atomicNumber);

            if (state.parentSlotIndex >= 0 && state.parentSlotIndex < (int)parentElement.bondingSlots.size()) {
                // --- ELASTIC COUPLING (Now handled by PhysicsEngine) ---
                // We no longer force positions directly here to allow for vibration and stress.
                // Simply update dockingProgress for visual animations.
                
                if (state.dockingProgress < 1.0f) {
                    state.dockingProgress += Config::BOND_DOCKING_SPEED;
                    if (state.dockingProgress > 1.0f) state.dockingProgress = 1.0f;
                }
                
                // Restoration forces are applied in PhysicsEngine::step
            }
        }
    }
}

void BondingSystem::updateSpontaneousBonding(std::vector<StateComponent>& states,
                                               std::vector<AtomComponent>& atoms,
                                               const std::vector<TransformComponent>& transforms,
                                               const SpatialGrid& grid,
                                               EnvironmentManager* env,
                                               int tractedEntityId) {
    
    // THROTTLING: Only run every N frames for performance (10 Hz)
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter < Config::BONDING_THROTTLE_FRAMES) return;
    frameCounter = 0;

    int tractedRoot = (tractedEntityId != -1) ? MathUtils::findMoleculeRoot(tractedEntityId, states) : -1;

    // O(N*k) OPTIMIZATION: Use SpatialGrid instead of O(N²) double loop
    for (int i = 1; i < (int)states.size(); i++) { 
        if (states[i].isClustered) continue; 

        // Skip atoms being dragged by tractor
        if (tractedRoot != -1 && MathUtils::findMoleculeRoot(i, states) == tractedRoot) continue;

        // Query only nearby atoms using spatial grid (O(k) where k ≈ 10)
        std::vector<int> neighbors = grid.getNearby({transforms[i].x, transforms[i].y}, Config::BOND_AUTO_RANGE * 1.5f);

        for (int j : neighbors) {
            if (j <= i) continue; // Avoid duplicate pairs
            if (j >= (int)states.size()) continue;

            float dx = transforms[i].x - transforms[j].x;
            float dy = transforms[i].y - transforms[j].y;
            float dz = transforms[i].z - transforms[j].z;
            float dist = MathUtils::length(dx, dy, dz);

            float rangeMultiplier = 1.0f;
            float angleMultiplier = 1.0f;
            if (env) {
                rangeMultiplier = env->getBondRangeMultiplier({transforms[i].x, transforms[i].y});
                angleMultiplier = env->getBondAngleMultiplier({transforms[i].x, transforms[i].y});
            }

            // CATALYSIS BOOST: If on active surface (Clay), extend range mostly for Ring Closures
            float currentRange = Config::BOND_AUTO_RANGE * rangeMultiplier;
            
            // Check potential cycle early to decide effective range
            // We can't know for sure without checking molecule root, but if range is boosted, we process more pairs.
            // Simplified: We accept checks up to 3x range if on Clay, filter later.
            if (rangeMultiplier > 1.2f) currentRange *= 3.0f; // Massive boost for Clay

            if (dist < currentRange) {
                // EXCLUSION: Ignore player molecule (ID 0) and tracted molecules for MERGES
                int rootI = MathUtils::findMoleculeRoot(i, states);
                int rootJ = MathUtils::findMoleculeRoot(j, states);

                bool isSameMolecule = (rootI == rootJ);
                
                // DEBUG: Log every pair being considered
                static int pairLogCounter = 0;
                if (++pairLogCounter > 300) {
                    TraceLog(LOG_DEBUG, "[BONDING FLOW] Pair %d-%d | Dist: %.1f | Roots: %d-%d | Same: %s", 
                             i, j, dist, rootI, rootJ, isSameMolecule ? "YES" : "NO");
                    pairLogCounter = 0;
                }
                
                // If we are using the Extended Clay Range (dist > normal), we MUST only allow Cycles.
                // Otherwise everything merges from far away.
                bool isExtendedRange = (dist > Config::BOND_AUTO_RANGE * rangeMultiplier);
                if (isExtendedRange && (!isSameMolecule)) continue; // Don't merge from far away, only close rings.

                // If bonding different molecules (Merging), enforce safety checks (don't merge what I'm holding)
                if (!isSameMolecule) {
                    if (rootI == 0 || rootJ == 0) continue;
                    if (tractedRoot != -1 && (rootI == tractedRoot || rootJ == tractedRoot)) continue;
                    
                    // VALENCY SHIELD: If root is shielded, no bonding allowed
                    if (states[rootI].isShielded || states[rootJ].isShielded) continue;
                }
                // However, if isSameMolecule (Ring Closure), we ALLOW it even if tracked/held.
                // This lets the player manualy bend a chain to close it.

                if (rootI != rootJ) {
                    // DIFFERENT MOLECULES -> NORMAL BOND (Merge)
                    if (tryBond(i, j, states, atoms, transforms, false, angleMultiplier) == SUCCESS) {
                        break; // One bond per atom per tick
                    }
                } else {
                    // SAME MOLECULE -> Log that we reached cycle detection
                    TraceLog(LOG_INFO, "[CYCLE FLOW] Same molecule detected: %d-%d (Root: %d)", i, j, rootI);
                    // SAME MOLECULE -> POTENTIAL CYCLE CLOSURE (Membrane Logic)
                    // Rule: Only close if they are "far" in terms of graph hops (> 4 atoms away)
                    // but close in space.
                    
                    // Simple BFS or limited traversal to check hop distance
                    // (Optimization: In a tree, unique path exists. We can trace upwards to LCA)
                    
                    // QUICK CHECK: Are they already directly bonded?
                    if (states[i].parentEntityId == j || states[j].parentEntityId == i) continue;
                    if (states[i].cycleBondId == j || states[j].cycleBondId == i) continue; // Already cycle bonded

                    // SIMPLIFIED CYCLE CHECK: Just check if both atoms have exactly 1 bond (terminals)
                    // This is much simpler than hop counting and directly matches our Phase 32 logic.
                    int bondsI = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
                    int bondsJ = (states[j].parentEntityId != -1 ? 1 : 0) + states[j].childCount;
                    
                    // LOG: Show bond counts for ALL same-molecule pairs (helps debug)
                    TraceLog(LOG_DEBUG, "[CYCLE CHECK] Pair %d-%d | BondsI: %d, BondsJ: %d", i, j, bondsI, bondsJ);
                    
                    // RELAXED CHECK: Allow cycle if EITHER is terminal OR if they're far in the chain
                    // For C4 chains: C1(1 bond) - C2(2) - C3(2) - C4(1 bond)
                    // We want C1 and C4 to close.
                    // Additionally, allow if BOTH have available valency (less than max bonds)
                    if (bondsI == 1 && bondsJ == 1) {
                        TraceLog(LOG_INFO, "[CYCLE DEBUG] Terminal pair found: %d (bonds=%d) <-> %d (bonds=%d) | Dist: %.1f", i, bondsI, j, bondsJ, dist);
                        
                        // FIX: Actually bond them physically using tryCycleBond
                        BondError result = tryCycleBond(i, j, states, atoms, transforms);
                         
                        if (result == SUCCESS) {
                            TraceLog(LOG_INFO, "[MEMBRANE] CYCLE FORMED: Atom %d - %d (Phys + Visual)", i, j);
                            MissionManager::getInstance().notifyBondCreated(atoms[i].atomicNumber, atoms[j].atomicNumber);
                            break;
                        } else {
                            TraceLog(LOG_WARNING, "[CYCLE DEBUG] tryCycleBond FAILED for %d-%d. Result: %d", i, j, result);
                        }
                    } else {
                        // LOG: Explain why this pair was skipped
                        static int skipLogCounter = 0;
                        if (++skipLogCounter > 200) {
                            TraceLog(LOG_DEBUG, "[CYCLE SKIP] Pair %d-%d not terminals (need bondsI=1, bondsJ=1)", i, j);
                            skipLogCounter = 0;
                        }
                    }
                }
            }
        }
    }
}
void BondingSystem::breakBond(int entityId, std::vector<StateComponent>& states, 
                              std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;
    StateComponent& state = states[entityId];

    if (!state.isClustered || state.parentEntityId == -1) return;

    int parentId = state.parentEntityId;

    // --- REVERT POLARITY ---
    float enHost = ChemistryDatabase::getInstance().getElement(atoms[parentId].atomicNumber).electronegativity;
    float enSource = ChemistryDatabase::getInstance().getElement(atoms[entityId].atomicNumber).electronegativity;
    float polarity = (enHost - enSource) * Config::POLARITY_FACTOR; 

    atoms[parentId].partialCharge -= polarity;
    atoms[entityId].partialCharge += polarity;

    // --- RELEASE BOND ---
    // Update parent's optimization fields (O(1))
    states[parentId].childCount--;
    states[parentId].occupiedSlots &= ~(1 << state.parentSlotIndex);

    state.isClustered = false;
    state.parentEntityId = -1;
    state.parentSlotIndex = -1;
    state.moleculeId = entityId; // Becomes its own root
    state.dockingProgress = 0.0f;

    TraceLog(LOG_INFO, "[BOND] Bond broken for atom %d (released from %d)", entityId, parentId);
}

int BondingSystem::findLastChild(int parentId, const std::vector<StateComponent>& states) {
    int lastChild = -1;
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isClustered && states[i].parentEntityId == parentId) {
            if (i > lastChild) {
                lastChild = i;
            }
        }
    }
    return lastChild;
}

int BondingSystem::findPrunableLeaf(int parentId, const std::vector<StateComponent>& states) {
    // Search for an atom that has parentId in its lineage (direct child)
    // but itself has NO children.
    
    int bestLeaf = -1;
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isClustered && states[i].parentEntityId != -1) {
            // Verify if this atom i belongs to parentId's structure
            // (Simplified: look for direct children of parentId that are leaves,
            // or recursively look for the deepest leaf)
                if (states[i].parentEntityId == parentId) {
                    // Is direct child. Check if it's a leaf (childCount == 0)
                    if (states[i].childCount == 0) {
                        // Is a direct leaf. Prefer the highest index (most recent)
                        if (i > bestLeaf) bestLeaf = i;
                    } else {
                        // If it has children, search recursively in that branch for the deepest leaf
                        int leafInBranch = findPrunableLeaf(i, states);
                        if (leafInBranch > bestLeaf) bestLeaf = leafInBranch;
                    }
                }
        }
    }
    return bestLeaf;
}

// --- CYCLE BONDING (Ring Closure) ---
BondingSystem::BondError BondingSystem::tryCycleBond(int i, int j, 
                               std::vector<StateComponent>& states, 
                               std::vector<AtomComponent>& atoms, 
                               const std::vector<TransformComponent>& transforms) {
    
    if (i < 0 || j < 0 || i == j) return INTERNAL_ERROR;
    
    // 1. Check if already cycle bonded
    if (states[i].cycleBondId != -1 || states[j].cycleBondId != -1) return ALREADY_BONDED;

    // 2. Find Slots for BOTH atoms (They must face each other)
    // Relative pos for I -> J
    Vector3 vecIJ = { transforms[j].x - transforms[i].x, transforms[j].y - transforms[i].y, transforms[j].z - transforms[i].z };
    int slotI = getBestAvailableSlot(i, vecIJ, states, atoms, true, 1.0f); // Forced (Ignore Angle)

    // Relative pos for J -> I
    Vector3 vecJI = { transforms[i].x - transforms[j].x, transforms[i].y - transforms[j].y, transforms[i].z - transforms[j].z };
    int slotJ = getBestAvailableSlot(j, vecJI, states, atoms, true, 1.0f); // Forced

    if (slotI != -1 && slotJ != -1) {
        // SUCCESS: Establish Cycle Bond
        states[i].cycleBondId = j;
        states[j].cycleBondId = i;
        
        // OCCUPY SLOTS
        states[i].occupiedSlots |= (1 << slotI);
        states[j].occupiedSlots |= (1 << slotJ);
        
        TraceLog(LOG_INFO, "[BOND] Cycle Bond Created: %d(Slot %d) <-> %d(Slot %d)", i, slotI, j, slotJ);
        return SUCCESS;
    }
    
    return VALENCY_FULL;
}

void BondingSystem::breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                                  std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;

    // 1. Break connection with parent
    if (states[entityId].parentEntityId != -1) {
        breakBond(entityId, states, atoms);
    }

    // 2. Break all connections with children
    // Iterate backwards to avoid potential (though unlikely) index issues
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isClustered && states[i].parentEntityId == entityId) {
            breakBond(i, states, atoms);
        }
    }

    TraceLog(LOG_INFO, "[BOND] Full Isolation: Atom %d released from all bonds", entityId);
}

