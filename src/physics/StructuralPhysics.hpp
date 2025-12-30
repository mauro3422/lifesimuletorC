#ifndef STRUCTURAL_PHYSICS_HPP
#define STRUCTURAL_PHYSICS_HPP

#include "../ecs/components.hpp"
#include "raylib.h"
#include <vector>

// Forward declaration back to global scope
class EnvironmentManager;

/**
 * Specialized system for structural dynamics, including:
 * - Rigid-body stability for rings.
 * - Smooth formation animation and handshake.
 * - Active folding and chemical affinity.
 */
namespace StructuralPhysics {

    /**
     * Applies rigid-body dynamics and formation logic to atoms in rings.
     */
    void applyRingDynamics(float dt, 
                          std::vector<TransformComponent>& transforms,
                          const std::vector<AtomComponent>& atoms,
                          std::vector<StateComponent>& states);

    /**
     * Applies folding forces to terminals and carbon affinity pulls.
     */
    void applyFoldingAndAffinity(float dt,
                                std::vector<TransformComponent>& transforms,
                                const std::vector<AtomComponent>& atoms,
                                std::vector<StateComponent>& states,
                                EnvironmentManager& environment);

}

#endif // STRUCTURAL_PHYSICS_HPP
