#include "PhysicsEngine.hpp"
#include "../core/Config.hpp"
#include <cmath>
#include <algorithm>

PhysicsEngine::PhysicsEngine() : grid(Config::GRID_CELL_SIZE) {}

void PhysicsEngine::step(float dt, std::vector<TransformComponent>& transforms) {
    // FUSIÓN DE LOOPS: Integración, Fricción y Límites en un solo paso
    for (TransformComponent& tr : transforms) {
        // 1. Integración
        tr.x += tr.vx * dt;
        tr.y += tr.vy * dt;
        tr.z += tr.vz * dt;

        // 2. Fricción Ambiental (Configurable)
        tr.vx *= Config::DRAG_COEFFICIENT;
        tr.vy *= Config::DRAG_COEFFICIENT;
        tr.vz *= Config::DRAG_COEFFICIENT;

        // 3. Límites del Mundo (Z)
        if (tr.z < Config::WORLD_DEPTH_MIN) {
            tr.z = Config::WORLD_DEPTH_MIN;
            tr.vz *= Config::WORLD_BOUNCE;
        } else if (tr.z > Config::WORLD_DEPTH_MAX) {
            tr.z = Config::WORLD_DEPTH_MAX;
            tr.vz *= Config::WORLD_BOUNCE;
        }
    }

    // Actualizar Grid Espacial
    grid.update(transforms);
}
