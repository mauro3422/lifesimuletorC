#ifndef BONDING_SYSTEM_HPP
#define BONDING_SYSTEM_HPP

#include "../ecs/components.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "raylib.h"
#include <vector>

class EnvironmentManager;
class SpatialGrid;

/**
 * BONDING SYSTEM (Deterministic)
 * Handles atom bonding into VSEPR slots and updating rigid molecular hierarchies.
 */
#include "BondingCore.hpp"
#include "MolecularHierarchy.hpp"
#include "RingChemistry.hpp"
#include "AutonomousBonding.hpp"
#include "UndoMechanism.hpp"

class BondingSystem {
public:
    using BondError = BondingCore::BondError;
    static constexpr BondError SUCCESS = BondingCore::SUCCESS;
    static constexpr BondError VALENCY_FULL = BondingCore::VALENCY_FULL;
    static constexpr BondError DISTANCE_TOO_FAR = BondingCore::DISTANCE_TOO_FAR;
    static constexpr BondError ANGLE_INCOMPATIBLE = BondingCore::ANGLE_INCOMPATIBLE;
    static constexpr BondError ALREADY_CLUSTERED = BondingCore::ALREADY_CLUSTERED;
    static constexpr BondError ALREADY_BONDED = BondingCore::ALREADY_BONDED;
    static constexpr BondError INTERNAL_ERROR = BondingCore::INTERNAL_ERROR;

    static bool canAcceptBond(int entityId, const std::vector<StateComponent>& states, const Element& element) {
        return BondingCore::canAcceptBond(entityId, states, element);
    }

    static BondError tryBond(int sourceId, int targetId, 
                       std::vector<StateComponent>& states,
                       std::vector<AtomComponent>& atoms,
                       const std::vector<TransformComponent>& transforms,
                       bool forced = false,
                       float angleMultiplier = 1.0f) {
        return BondingCore::tryBond(sourceId, targetId, states, atoms, transforms, forced, angleMultiplier);
    }

    static void updateHierarchy(std::vector<TransformComponent>& transforms,
                               std::vector<StateComponent>& states,
                               const std::vector<AtomComponent>& atoms);

    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         std::vector<AtomComponent>& atoms,
                                         std::vector<TransformComponent>& transforms,
                                         const class SpatialGrid& grid,
                                         const std::vector<int>& rootCache,
                                         EnvironmentManager* env = nullptr,
                                         int tractedEntityId = -1) {
        AutonomousBonding::updateSpontaneousBonding(states, atoms, transforms, grid, rootCache, env, tractedEntityId);
    }

    static void breakBond(int entityId, std::vector<StateComponent>& states, 
                          std::vector<AtomComponent>& atoms) {
        BondingCore::breakBond(entityId, states, atoms);
    }

    static void breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                               std::vector<AtomComponent>& atoms);

    static int findLastChild(int parentId, const std::vector<StateComponent>& states) {
        return UndoMechanism::findLastChild(parentId, states);
    }

    static int findPrunableLeaf(int parentId, const std::vector<StateComponent>& states) {
        return UndoMechanism::findPrunableLeaf(parentId, states);
    }

    static int getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle = false,
                                   float angleMultiplier = 1.0f) {
        return BondingCore::getBestAvailableSlot(parentId, relativePos, states, atoms, ignoreAngle, angleMultiplier);
    }

    static int getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states,
                                const std::vector<AtomComponent>& atoms) {
        return BondingCore::getFirstFreeSlot(parentId, states, atoms);
    }

    static BondError tryCycleBond(int i, int j, std::vector<StateComponent>& states, 
                                 std::vector<AtomComponent>& atoms, 
                                 std::vector<TransformComponent>& transforms) {
        return RingChemistry::tryCycleBond(i, j, states, atoms, transforms);
    }

    static void propagateMoleculeId(int entityId, int newMoleculeId, std::vector<StateComponent>& states) {
        MolecularHierarchy::propagateMoleculeId(entityId, newMoleculeId, states);
    }
};

#endif
