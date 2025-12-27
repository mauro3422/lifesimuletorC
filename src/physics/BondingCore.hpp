#ifndef BONDING_CORE_HPP
#define BONDING_CORE_HPP

#include <vector>
#include <cassert>
#include <cmath>
#include "raylib.h"
#include "raymath.h"
#include "../ecs/components.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../core/Config.hpp"
#include "MolecularHierarchy.hpp"

/**
 * BondingCore (Phase 30)
 * Atomic operations for creating and breaking bonds.
 */
class BondingCore {
public:
    enum BondError {
        SUCCESS = 0,
        VALENCY_FULL,
        DISTANCE_TOO_FAR,
        ANGLE_INCOMPATIBLE,
        ALREADY_CLUSTERED,
        ALREADY_BONDED,
        INTERNAL_ERROR
    };

    static bool canAcceptBond(int entityId, const std::vector<StateComponent>& states, const Element& element) {
        if (entityId < 0 || entityId >= (int)states.size()) return false;
        int currentBonds = (states[entityId].parentEntityId != -1 ? 1 : 0) + states[entityId].childCount;
        return currentBonds < element.maxBonds;
    }

    static int getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
        if (parentId < 0 || parentId >= (int)states.size()) return -1;

        const Element& element = ChemistryDatabase::getInstance().getElement(atoms[parentId].atomicNumber);
        int currentBonds = (states[parentId].parentEntityId != -1 ? 1 : 0) + states[parentId].childCount;

        if (currentBonds >= element.maxBonds) return -1;

        for (int i = 0; i < (int)element.bondingSlots.size(); i++) {
            assert(i < 32 && "Slot index out of range for uint32_t occupiedSlots");
            if (!(states[parentId].occupiedSlots & (1u << i))) return i;
        }
        return -1;
    }

    static int getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle = false,
                                   float angleMultiplier = 1.0f) {
        if (parentId < 0 || parentId >= (int)states.size()) return -1;

        const Element& element = ChemistryDatabase::getInstance().getElement(atoms[parentId].atomicNumber);
        int currentBonds = (states[parentId].parentEntityId != -1 ? 1 : 0) + states[parentId].childCount;

        if (currentBonds >= element.maxBonds) return -1;

        float minDist = 1e30f;
        int bestSlot = -1;

        Vector3 normRelPos = Vector3Normalize(relativePos);

        for (int i = 0; i < (int)element.bondingSlots.size(); i++) {
            if (!(states[parentId].occupiedSlots & (1u << i))) {
                Vector3 slotDir = element.bondingSlots[i];
                float dot = Vector3DotProduct(normRelPos, slotDir);

                if (ignoreAngle || dot > (0.6f * angleMultiplier)) {
                    float d = Vector3Distance(normRelPos, slotDir);
                    if (d < minDist) {
                        minDist = d;
                        bestSlot = i;
                    }
                }
            }
        }
        return bestSlot;
    }

    static BondError tryBond(int sourceId, int targetId, 
                       std::vector<StateComponent>& states,
                       std::vector<AtomComponent>& atoms,
                       const std::vector<TransformComponent>& transforms,
                       bool forced = false,
                       float angleMultiplier = 1.0f) {
        if (sourceId < 0 || targetId < 0 || sourceId == targetId) return INTERNAL_ERROR;
        if (states[sourceId].isLocked()) return ALREADY_CLUSTERED;

        int molRootId = MolecularHierarchy::findRoot(targetId, states);
        
        std::vector<int> candidates;
        for (int i = 0; i < (int)states.size(); i++) {
            if (MolecularHierarchy::findRoot(i, states) == molRootId) {
                candidates.push_back(i);
            }
        }

        int bestHostId = -1;
        int bestSlotIdx = -1;
        float minSourceDist = Config::FLOAT_MAX;

        for (int hostId : candidates) {
            const TransformComponent& hostTr = transforms[hostId];
            const TransformComponent& sourceTr = transforms[sourceId];
            Vector3 relPos = { sourceTr.x - hostTr.x, sourceTr.y - hostTr.y, sourceTr.z - hostTr.z };

            int slotIdx = getBestAvailableSlot(hostId, relPos, states, atoms, forced, angleMultiplier);
            if (slotIdx == -1 && forced) {
                slotIdx = getFirstFreeSlot(hostId, states, atoms);
            }

            if (slotIdx != -1) {
                float dist = Vector3Length(relPos);
                if (dist < minSourceDist) {
                    minSourceDist = dist;
                    bestHostId = hostId;
                    bestSlotIdx = slotIdx;
                }
            }
        }

        if (bestHostId != -1) {
            states[sourceId].isClustered = true;
            states[sourceId].parentEntityId = bestHostId; 
            states[sourceId].parentSlotIndex = bestSlotIdx;
            states[sourceId].moleculeId = molRootId; 
            states[sourceId].dockingProgress = 0.0f; 

            states[bestHostId].childCount++;
            states[bestHostId].occupiedSlots |= (1u << bestSlotIdx);

            MolecularHierarchy::propagateMoleculeId(sourceId, molRootId, states);
            return SUCCESS;
        }

        return INTERNAL_ERROR;
    }

    static void breakBond(int entityId, std::vector<StateComponent>& states, std::vector<AtomComponent>& atoms) {
        if (entityId < 0 || entityId >= (int)states.size() || !states[entityId].isClustered) return;

        int parentId = states[entityId].parentEntityId;
        if (parentId != -1) {
            states[parentId].childCount--;
            states[parentId].occupiedSlots &= ~(1u << states[entityId].parentSlotIndex);
        }

        states[entityId].isClustered = false;
        states[entityId].parentEntityId = -1;
        states[entityId].moleculeId = entityId; // Each isolated atom is its own root
        states[entityId].dockingProgress = 0.0f;

        MolecularHierarchy::propagateMoleculeId(entityId, entityId, states);
    }
};

#endif // BONDING_CORE_HPP
