#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "raylib.h"
#include "../ecs/components.hpp"
#include "TractorBeam.hpp"

// Forward declaration para evitar circulares
class SpatialGrid;

/**
 * CLASE JUGADOR
 */
class Player {
public:
    Player(int entityIndex);

    // Ahora recibe la grilla para sus herramientas
    void update(float dt, const class InputHandler& input, 
                std::vector<TransformComponent>& worldTransforms, 
                const Camera2D& camera,
                const SpatialGrid& grid,
                std::vector<StateComponent>& states,
                const std::vector<AtomComponent>& atoms);

    void applyPhysics(std::vector<TransformComponent>& worldTransforms,
                      std::vector<StateComponent>& states,
                      const std::vector<AtomComponent>& atoms); 
    
    TractorBeam& getTractor() { return tractor; }
    int getAtomicNumber() const { return atomicNumber; }
    float getZoomTarget() const { return 2.5f; }
    int getEntityIndex() const { return playerIndex; }

private:
    int playerIndex;
    TractorBeam tractor;
    int atomicNumber;
    float speed;
};

#endif
