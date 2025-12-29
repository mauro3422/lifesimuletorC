#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include "../ecs/components.hpp"
#include "SpatialGrid.hpp"
#include "../world/EnvironmentManager.hpp"
#include <vector>

/**
 * PHYSICS ENGINE
 * Orchestrates simulation subsystems: forces, bonding, collisions, and environment.
 */
class PhysicsEngine {
public:
    PhysicsEngine();
    
    // Main simulation step
    void step(float dt, std::vector<TransformComponent>& transforms,
              std::vector<AtomComponent>& atoms,
              std::vector<StateComponent>& states,
              const class ChemistryDatabase& db,
              int tractedEntityId = -1);

    // Grid access for other systems (e.g., TractorBeam)
    const SpatialGrid& getGrid() const { return grid; }

    EnvironmentManager& getEnvironment() { return environment; }

private:
    void resolveCollisions(std::vector<TransformComponent>& transforms);
    
    // Helper methods extracted from step() for better maintainability
    void validateRingIntegrity(std::vector<StateComponent>& states);
    
    void applyCoulombForces(float dt,
                            std::vector<TransformComponent>& transforms,
                            const std::vector<AtomComponent>& atoms,
                            const class ChemistryDatabase& db);
    
    void applyBondSprings(float dt,
                          std::vector<TransformComponent>& transforms,
                          const std::vector<AtomComponent>& atoms,
                          std::vector<StateComponent>& states,
                          const class ChemistryDatabase& db,
                          int diagCounter);
    
    void applyCycleBonds(float dt,
                         std::vector<TransformComponent>& transforms,
                         const std::vector<AtomComponent>& atoms,
                         const std::vector<StateComponent>& states,
                         const class ChemistryDatabase& db);
    
    void integrateMotion(float dt,
                         std::vector<TransformComponent>& transforms,
                         const std::vector<StateComponent>& states);
    
    SpatialGrid grid;
    EnvironmentManager environment;
};

#endif
