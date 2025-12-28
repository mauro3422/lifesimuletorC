#ifndef TRACTOR_BEAM_HPP
#define TRACTOR_BEAM_HPP

#include "raylib.h"
#include "../ecs/components.hpp"
#include "../physics/SpatialGrid.hpp"
#include <vector>

/**
 * TRACTOR BEAM MODULE (Optimized)
 * Handles atom capture and dragging with O(1) spatial grid lookups.
 */
class TractorBeam {
public:
    TractorBeam() : targetIndex(-1), active(false), wasActiveLastFrame(false), isNewCapture(false) {}

    // Uses spatial grid for O(1) neighbor lookup
    void update(const Vector2& mouseWorldPos, bool isInputActive, 
                const std::vector<TransformComponent>& transforms,
                const std::vector<StateComponent>& states,
                const std::vector<AtomComponent>& atoms,
                const SpatialGrid& grid);
    
    int getTargetIndex() const { return targetIndex; }
    Vector2 getTargetPosition() const { return targetPos; }
    bool isActive() const { return active && targetIndex != -1; }
    bool becameActive() const { return isNewCapture; } // Returns true on the frame a capture occurred
    void release() { active = false; targetIndex = -1; isNewCapture = false; }

private:
    int targetIndex; 
    bool active;
    bool wasActiveLastFrame;
    bool isNewCapture;
    Vector2 targetPos;
};

#endif
