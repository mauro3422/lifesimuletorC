#include "BondingSystem.hpp"

// Subsystem Includes (Implementation Only)
#include "BondingCore.hpp"
#include "AutonomousBonding.hpp"
#include "MolecularHierarchy.hpp"
#include "RingChemistry.hpp"
#include "PruningUtils.hpp"
#include "../chemistry/ChemistryDatabase.hpp"

#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "raylib.h"
#include <cmath>
#include <algorithm>

// --- Facade Implementation ---

bool BondingSystem::canAcceptBond(int entityId, const std::vector<StateComponent>& states, const Element& element) {
    return ::BondingCore::canAcceptBond(entityId, states, element);
}

BondError BondingSystem::tryBond(int sourceId, int targetId, 
                   std::vector<StateComponent>& states,
                   std::vector<AtomComponent>& atoms,
                   const std::vector<TransformComponent>& transforms,
                   bool forced,
                   float angleMultiplier) {
    return (BondError)::BondingCore::tryBond(sourceId, targetId, states, atoms, transforms, forced, angleMultiplier);
}

void BondingSystem::updateHierarchy(std::vector<TransformComponent>& transforms,
                                   std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms) {
    for (int i = 0; i < (int)transforms.size(); i++) {
        StateComponent& state = states[i];
        if (state.isClustered && state.parentEntityId != -1) {
            // Simply update dockingProgress for visual animations.
            // Restoration forces are handled by PhysicsEngine::step.
            if (state.dockingProgress < 1.0f) {
                state.dockingProgress += Config::BOND_DOCKING_SPEED;
                if (state.dockingProgress > 1.0f) state.dockingProgress = 1.0f;
            }
        }
    }
}

void BondingSystem::updateSpontaneousBonding(std::vector<StateComponent>& states,
                                     std::vector<AtomComponent>& atoms,
                                     std::vector<TransformComponent>& transforms,
                                     const SpatialGrid& grid,
                                     const std::vector<int>& rootCache,
                                     EnvironmentManager* env,
                                     int tractedEntityId) {
    ::AutonomousBonding::updateSpontaneousBonding(states, atoms, transforms, grid, rootCache, env, tractedEntityId);
}

void BondingSystem::breakBond(int entityId, std::vector<StateComponent>& states, 
                      std::vector<AtomComponent>& atoms) {
    ::BondingCore::breakBond(entityId, states, atoms);
}

void BondingSystem::breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                                  std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;

    // 1. Break cycle bond if exists - and clean up ALL ring members using centralized invalidation
    if (states[entityId].cycleBondId != -1 || states[entityId].isInRing) {
        int ringId = states[entityId].ringInstanceId;
        RingChemistry::invalidateRing(ringId, states);
    }

    // 2. Break connection with parent
    if (states[entityId].isClustered) {
        ::BondingCore::breakBond(entityId, states, atoms);
    }

    // 3. Break connections with children
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].parentEntityId == entityId) {
            ::BondingCore::breakBond(i, states, atoms);
        }
    }
}

int BondingSystem::findLastChild(int parentId, const std::vector<StateComponent>& states) {
    return ::PruningUtils::findLastChild(parentId, states);
}

int BondingSystem::findPrunableLeaf(int parentId, const std::vector<StateComponent>& states) {
    return ::PruningUtils::findPrunableLeaf(parentId, states);
}

int BondingSystem::getBestAvailableSlot(int parentId, Vector3 relativePos,
                               const std::vector<StateComponent>& states,
                               const std::vector<AtomComponent>& atoms,
                               bool ignoreAngle,
                               float angleMultiplier) {
    return ::BondingCore::getBestAvailableSlot(parentId, relativePos, states, atoms, ignoreAngle, angleMultiplier);
}

int BondingSystem::getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states,
                            const std::vector<AtomComponent>& atoms) {
    return ::BondingCore::getFirstFreeSlot(parentId, states, atoms);
}

BondError BondingSystem::tryCycleBond(int i, int j, std::vector<StateComponent>& states, 
                             std::vector<AtomComponent>& atoms, 
                             std::vector<TransformComponent>& transforms) {
    return (BondError)::RingChemistry::tryCycleBond(i, j, states, atoms, transforms);
}

void BondingSystem::propagateMoleculeId(int entityId, int newMoleculeId, std::vector<StateComponent>& states) {
    ::MolecularHierarchy::propagateMoleculeId(entityId, newMoleculeId, states);
}
