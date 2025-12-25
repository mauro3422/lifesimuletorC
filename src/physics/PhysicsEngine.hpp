#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include "../ecs/components.hpp"
#include "SpatialGrid.hpp"
#include <vector>

/**
 * El Motor de Física orquestará los diferentes módulos.
 */
class PhysicsEngine {
public:
    PhysicsEngine();
    
    // El paso de simulación principal
    void step(float dt, std::vector<TransformComponent>& transforms,
              const std::vector<AtomComponent>& atoms,
              const std::vector<StateComponent>& states);

    // Acceso a la grilla para otros sistemas (como el TractorBeam)
    const SpatialGrid& getGrid() const { return grid; }

private:
    void resolveCollisions(std::vector<TransformComponent>& transforms);
    
    SpatialGrid grid;
};

#endif
