#include "TractorBeam.hpp"
#include "../core/Config.hpp"
#include <cmath>

void TractorBeam::update(const Vector2& mouseWorldPos, bool isInputActive, 
                         const std::vector<TransformComponent>& transforms,
                         const SpatialGrid& grid) {
    active = isInputActive;

    if (!active) {
        targetIndex = -1;
        return;
    }

    if (targetIndex != -1) {
        return; 
    }

    // BUSQUEDA OPTIMIZADA: Solo miramos átomos cerca del mouse usando la grilla
    float range = Config::TRACTOR_PICKUP_RANGE;
    std::vector<int> nearby = grid.getNearby(mouseWorldPos, range);
    
    // DEBUG: Ver cuántos átomos detecta
    if (nearby.size() > 0) {
        TraceLog(LOG_DEBUG, "[TRACTOR] Detectados %d atomos cerca del mouse", (int)nearby.size());
    }
    
    float minSourceDist = range;
    int bestIdx = -1;

    for (int i : nearby) {
        if (i == 0) continue; // Ignorar al jugador (índice 0)
        
        float dx = mouseWorldPos.x - transforms[i].x;
        float dy = mouseWorldPos.y - transforms[i].y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < minSourceDist) {
            minSourceDist = dist;
            bestIdx = i;
        }
    }

    if (bestIdx != -1) {
        TraceLog(LOG_INFO, "[TRACTOR] Capturado atomo ID %d a distancia %.2f", bestIdx, minSourceDist);
    }

    targetIndex = bestIdx;
}
