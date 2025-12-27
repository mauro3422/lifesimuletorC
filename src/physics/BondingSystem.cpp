#include "BondingSystem.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include "raylib.h"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"

#include "../chemistry/ChemistryDatabase.hpp"

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

void BondingSystem::breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                                  std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;

    // 1. Break cycle bond if exists
    if (states[entityId].cycleBondId != -1) {
        int partnerId = states[entityId].cycleBondId;
        if (partnerId >= 0 && partnerId < (int)states.size()) {
            states[partnerId].cycleBondId = -1;
            states[partnerId].isInRing = false;
        }
        states[entityId].cycleBondId = -1;
        states[entityId].isInRing = false;
    }

    // 2. Break connection with parent
    if (states[entityId].isClustered) {
        BondingCore::breakBond(entityId, states, atoms);
    }

    // 3. Break connections with children
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].parentEntityId == entityId) {
            BondingCore::breakBond(i, states, atoms);
        }
    }
}
