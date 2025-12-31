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
                                             EnvironmentManager* env,
                                             int tractedEntityId) {
    ::AutonomousBonding::updateSpontaneousBonding(states, atoms, transforms, grid, env, tractedEntityId);
}

void BondingSystem::breakBond(int entityId, std::vector<StateComponent>& states, 
                      std::vector<AtomComponent>& atoms) {
    ::BondingCore::breakBond(entityId, states, atoms);
}

void BondingSystem::breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                                  std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;

    TraceLog(LOG_INFO, "[BOND_SYSTEM] Isolating Atom %d...", entityId);

    // 1. Break cycle bond if exists - and clean up ALL ring members using centralized invalidation
    if (states[entityId].cycleBondId != -1 || states[entityId].isInRing) {
        int ringId = states[entityId].ringInstanceId;
        TraceLog(LOG_INFO, "  - Breaking Ring %d", ringId);
        RingChemistry::invalidateRing(ringId, states);
    }

    // 2. Break connection with parent
    if (states[entityId].isClustered) {
        TraceLog(LOG_INFO, "  - Breaking Parent Bond");
        ::BondingCore::breakBond(entityId, states, atoms);
    }

    // 3. Break connections with children
    int breakCount = 0;
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].parentEntityId == entityId) {
            TraceLog(LOG_INFO, "  - Found Child: Atom %d", i);
            ::BondingCore::breakBond(i, states, atoms);
            breakCount++;
        }
    }

    // 4. Phase 43 FIX: Fully isolate this atom (reset moleculeId to self)
    states[entityId].moleculeId = entityId;
    states[entityId].isClustered = false;
    states[entityId].parentEntityId = -1;
    
    // Phase 44 FIX: Clear stale child references
    states[entityId].childList.clear();
    states[entityId].childCount = 0;
    states[entityId].occupiedSlots = 0;
    
    // Use centralized ring flag clearing
    RingChemistry::clearRingFlags(entityId, states);

    TraceLog(LOG_INFO, "[BOND_SYSTEM] Isolation of %d complete. Broke %d child bonds.", entityId, breakCount);
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

void BondingSystem::propagateMoleculeId(int entityId, std::vector<StateComponent>& states) {
    ::MolecularHierarchy::propagateMoleculeId(entityId, states);
}
