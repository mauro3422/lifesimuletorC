#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include "../ecs/components.hpp"
#include "SpatialGrid.hpp"
#include "../world/EnvironmentManager.hpp"
#include <vector>

/**
 * El Motor de Física orquestará los diferentes módulos.
 */
class PhysicsEngine {
public:
    PhysicsEngine();
    
    // El paso de simulación principal
    void step(float dt, std::vector<TransformComponent>& transforms,
              std::vector<AtomComponent>& atoms,
              std::vector<StateComponent>& states,
              int tractedEntityId = -1);

    // Acceso a la grilla para otros sistemas (como el TractorBeam)
    const SpatialGrid& getGrid() const { return grid; }

    EnvironmentManager& getEnvironment() { return environment; }

private:
    void resolveCollisions(std::vector<TransformComponent>& transforms);
    
    SpatialGrid grid;
    EnvironmentManager environment;
};

#endif
