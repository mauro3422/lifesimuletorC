#ifndef TRACTOR_BEAM_HPP
#define TRACTOR_BEAM_HPP

#include "raylib.h"
#include "../ecs/components.hpp"
#include "../physics/SpatialGrid.hpp"
#include <vector>

/**
 * MODULO DE TRACTOR BEAM (Optimizado)
 */
class TractorBeam {
public:
    TractorBeam() : targetIndex(-1), active(false), wasActiveLastFrame(false), isNewCapture(false) {}

    // Ahora recibe la grilla para una búsqueda veloz O(1)
    void update(const Vector2& mouseWorldPos, bool isInputActive, 
                const std::vector<TransformComponent>& transforms,
                const SpatialGrid& grid);
    
    int getTargetIndex() const { return targetIndex; }
    Vector2 getTargetPosition() const { return targetPos; }
    bool isActive() const { return active && targetIndex != -1; }
    bool becameActive() const { return isNewCapture; } // Notifica si se acaba de capturar algo
    void release() { active = false; targetIndex = -1; isNewCapture = false; }

private:
    int targetIndex; 
    bool active;
    bool wasActiveLastFrame;
    bool isNewCapture;
    Vector2 targetPos;
};

#endif
