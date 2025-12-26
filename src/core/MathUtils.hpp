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

    // Scans a molecular cluster and returns its atomic composition
    inline std::map<int, int> scanMoleculeComposition(int entityId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
        std::map<int, int> composition;
        if (entityId < 0 || entityId >= (int)states.size()) return composition;

        int rootId = findMoleculeRoot(entityId, states);
        if (rootId == -1) return composition;

        // Traverse all entities to find those belonging to the same root
        // This is more robust than relying on the moleculeId field which might be out of sync.
        for (int i = 0; i < (int)states.size(); i++) {
            // A cluster member is anyone who shares the same root ID.
            // The root ID itself (parentEntityId == -1) must be included too.
            if (findMoleculeRoot(i, states) == rootId) {
                composition[atoms[i].atomicNumber]++;
            }
        }
        return composition;
    }
}

#endif // MATH_UTILS_HPP
