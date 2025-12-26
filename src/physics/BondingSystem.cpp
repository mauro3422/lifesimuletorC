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

        TraceLog(LOG_INFO, "[BOND] GLOBAL SUCCESS: %d -> %d (Molecule %d) Polarity: %.2f", sourceId, bestHostId, molRootId, polarity);
        
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
                    
                    TraceLog(LOG_INFO, "[BOND] UNIVERSAL SPLICE: Atom %d inserted as bridge in molecule %d", sourceId, molRootId);
                    
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

            if (dist < Config::BOND_AUTO_RANGE * rangeMultiplier) {
                // EXCLUSION: Ignore player molecule (ID 0) and tracted molecules
                int rootI = MathUtils::findMoleculeRoot(i, states);
                int rootJ = MathUtils::findMoleculeRoot(j, states);

                if (rootI == 0 || rootJ == 0) continue;
                if (tractedRoot != -1 && (rootI == tractedRoot || rootJ == tractedRoot)) continue;
                
                // VALENCY SHIELD: If root is shielded, no bonding allowed
                if (states[rootI].isShielded || states[rootJ].isShielded) continue;

                if (rootI != rootJ) {
                    // DIFFERENT MOLECULES -> NORMAL BOND (Merge)
                    if (tryBond(i, j, states, atoms, transforms, false, angleMultiplier) == SUCCESS) {
                        break; // One bond per atom per tick
                    }
                } else {
                    // SAME MOLECULE -> POTENTIAL CYCLE CLOSURE (Membrane Logic)
                    // Rule: Only close if they are "far" in terms of graph hops (> 4 atoms away)
                    // but close in space.
                    
                    // Simple BFS or limited traversal to check hop distance
                    // (Optimization: In a tree, unique path exists. We can trace upwards to LCA)
                    
                    // QUICK CHECK: Are they already directly bonded?
                    if (states[i].parentEntityId == j || states[j].parentEntityId == i) continue;
                    if (states[i].cycleBondId == j || states[j].cycleBondId == i) continue; // Already cycle bonded

                    // DISTANCE CHECK (Graph Hops) - Simplified "Tracer"
                    // Trace parents up to root or until meeting
                    int pI = i; 
                    int hopsI = 0;
                    while (states[pI].parentEntityId != -1 && hopsI < 8) {
                        pI = states[pI].parentEntityId;
                        hopsI++;
                    }
                    
                    int pJ = j;
                    int hopsJ = 0;
                    while (states[pJ].parentEntityId != -1 && hopsJ < 8) {
                        pJ = states[pJ].parentEntityId;
                        hopsJ++;
                    }

                    // Strict C4Si Rule: Need at least 4 atoms in the chain to loop.
                    // If total graph distance (approx) > 4
                    // Note: This is a heuristic. For exact, we'd need full LCA.
                    // Assuming balanced chains or long tail.
                    
                    if (hopsI + hopsJ >= 4) {
                        // CLOSE THE RING
                        states[i].cycleBondId = j;
                        // states[j].cycleBondId = i; // Optional: One-way reference is enough for spring physics, simpler to manage.
                        
                        TraceLog(LOG_INFO, "[MEMBRANE] CYCLE FORMED: Atom %d - %d (Ring Closure)", i, j);
                        
                        // NOTIFY MISSION or ACHIEVEMENT here
                        break; 
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

