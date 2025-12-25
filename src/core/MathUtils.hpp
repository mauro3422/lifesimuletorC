#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include "raylib.h"
#include <vector>
#include "../ecs/components.hpp"

namespace MathUtils {
    // Generates a random jitter between -1.0 and 1.0
    inline float getJitter() {
        return (float)GetRandomValue(-100, 100) / 100.0f;
    }

    // Finds the root of a molecular structure given an entity index
    inline int findMoleculeRoot(int entityId, const std::vector<StateComponent>& states) {
        if (entityId < 0 || entityId >= (int)states.size()) return -1;
        
        int rootId = entityId;
        int safetyCounter = 0;
        const int MAX_DEPTH = 100; // Prevent infinite loops
        
        while (states[rootId].parentEntityId != -1 && safetyCounter < MAX_DEPTH) {
            rootId = states[rootId].parentEntityId;
            safetyCounter++;
        }
        
        return rootId;
    }
}

#endif // MATH_UTILS_HPP
