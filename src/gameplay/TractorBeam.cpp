#include "TractorBeam.hpp"
#include "../core/MathUtils.hpp"
#include "../core/Config.hpp"
#include <cmath>

#include "../chemistry/ChemistryDatabase.hpp"

void TractorBeam::update(const Vector2& mouseWorldPos, bool isInputActive, 
                         const std::vector<TransformComponent>& transforms,
                         const std::vector<StateComponent>& states,
                         const std::vector<AtomComponent>& atoms,
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
    
    // Z-DEPTH SORTING ALGORITHM
    // Prioritize atoms physically "closer" to the camera (higher Z index + radius logic)
    // For now, simpler heuristic: Pick cleanest match.
    // If multiple atoms are under the cursor, pick the one with highest index? Or closest distance2D?
    // Closest 2D distance is usually best for "feel" unless they overlap perfectly.
    // If they overlap, Z should win.
    
    // Improved Selection:
    // float bestScore = -1.0f; 

    for (int i : nearby) {
        if (i == 0) continue; // Ignore player

        float distSq = MathUtils::distSq(mouseWorldPos, {transforms[i].x, transforms[i].y});
        if (distSq > range * range) continue;

        // Score formula: Closer is better, Higher Z is better.
        // Score = (1.0 / dist) * (1.0 + z_bonus)
        // But let's keep it simple: Just closest 2D distance for now, BUT if very close, check Z?
        // Actually, user compliant is "detecto otros atomos medio lejos o atras".
        
        // Strict distance check
        float dist = sqrtf(distSq);
        
        if (dist < minSourceDist) {
            minSourceDist = dist;
            bestIdx = i;
        }
    }

    if (bestIdx != -1) {
        // --- SMART LOGGING: IDENTIFY MOLECULE ---
        int molId = states[bestIdx].moleculeId;
        const auto& mol = ChemistryDatabase::getInstance().getMoleculeById(molId);
        const char* molName = mol ? mol->name.c_str() : "Unknown";
        const char* atomName = ChemistryDatabase::getInstance().getElement(atoms[bestIdx].atomicNumber).symbol.c_str();

        TraceLog(LOG_INFO, "[TRACTOR] Captured %s (in %s) [ID: %d]", atomName, molName, bestIdx);
        isNewCapture = true;
    }

    targetIndex = bestIdx;
}
