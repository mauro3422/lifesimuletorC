#include "TractorBeam.hpp"
#include "../core/MathUtils.hpp"
#include "../core/Config.hpp"
#include <cmath>

void TractorBeam::update(const Vector2& mouseWorldPos, bool isInputActive, 
                         const std::vector<TransformComponent>& transforms,
                         const SpatialGrid& grid) {
    
    isNewCapture = false; // Reset per frame
    bool startedThisFrame = isInputActive && !wasActiveLastFrame;
    wasActiveLastFrame = isInputActive;

    active = isInputActive;

    if (!active) {
        targetIndex = -1;
        return;
    }

    targetPos = mouseWorldPos; 

    // TARGET LOCK: If we already have a target, keep the ID as long as active=true
    if (targetIndex != -1) {
        return; 
    }

    // ONLY SEARCH ON INITIAL CLICK (Prevents multiple or automatic captures)
    if (!startedThisFrame) {
        return;
    }

    // OPTIMIZED SEARCH: Only check atoms near the mouse using the spatial grid
    float range = Config::TRACTOR_PICKUP_RANGE;
    std::vector<int> nearby = grid.getNearby(mouseWorldPos, range);
    
    // DEBUG: Verify detection count
    if (nearby.size() > 0) {
        TraceLog(LOG_DEBUG, "[TRACTOR] Detected %d atoms near mouse", (int)nearby.size());
    }
    
    float minSourceDist = range;
    int bestIdx = -1;

    for (int i : nearby) {
        if (i == 0) continue; // Ignore player (index 0)
        
        float dist = MathUtils::dist(mouseWorldPos, {transforms[i].x, transforms[i].y});

        if (dist < minSourceDist) {
            minSourceDist = dist;
            bestIdx = i;
        }
    }

    if (bestIdx != -1) {
        TraceLog(LOG_INFO, "[TRACTOR] Captured atom ID %d at distance %.2f", bestIdx, minSourceDist);
        isNewCapture = true;
    }

    targetIndex = bestIdx;
}
