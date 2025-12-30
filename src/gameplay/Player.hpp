#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "raylib.h"
#include "../ecs/components.hpp"
#include "TractorBeam.hpp"
#include "UndoManager.hpp"

// Forward declarations
class SpatialGrid;
class InputHandler;

/**
 * PLAYER CLASS
 * Refactored for single responsibility: controls player movement.
 * Docking delegated to DockingSystem.
 * Undo delegated to UndoManager.
 * Tractor physics delegated to TractorBeam.
 */
class Player {
public:
    Player(int entityIndex);

    void update(float dt, const InputHandler& input, 
                std::vector<TransformComponent>& worldTransforms, 
                const Camera2D& camera,
                const SpatialGrid& grid,
                std::vector<StateComponent>& states,
                std::vector<AtomComponent>& atoms);

    void applyPhysics(std::vector<TransformComponent>& worldTransforms,
                      std::vector<StateComponent>& states,
                      std::vector<AtomComponent>& atoms); 
    
    TractorBeam& getTractor() { return tractor; }
    UndoManager& getUndoManager() { return undoManager; }
    int getAtomicNumber() const { return atomicNumber; }
    float getZoomTarget() const { return 2.5f; }
    int getEntityIndex() const { return playerIndex; }

private:
    int playerIndex;
    TractorBeam tractor;
    UndoManager undoManager;
    int atomicNumber;
    float speed;
    int lastRootId = -1;
};

#endif
