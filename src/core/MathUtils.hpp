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
    // O(depth) where depth is typically < 10
    inline int findMoleculeRoot(int entityId, const std::vector<StateComponent>& states) {
        if (entityId < 0 || entityId >= (int)states.size()) return -1;
        
        int rootId = entityId;
        int safetyCounter = 0;
        const int MAX_DEPTH = 100;
        
        while (states[rootId].parentEntityId != -1 && safetyCounter < MAX_DEPTH) {
            rootId = states[rootId].parentEntityId;
            safetyCounter++;
        }
        
        return rootId;
    }

    // Simple propagation - just set the immediate bonded atom's moleculeId
    // Called only when creating new bonds (not every frame)
    inline void setMoleculeId(int entityId, int moleculeId, std::vector<StateComponent>& states) {
        if (entityId < 0 || entityId >= (int)states.size()) return;
        states[entityId].moleculeId = moleculeId;
    }

    // Scans a molecular cluster and returns its atomic composition
    inline std::map<int, int> scanMoleculeComposition(int entityId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
        std::map<int, int> composition;
        if (entityId < 0 || entityId >= (int)states.size()) return composition;

        int rootId = findMoleculeRoot(entityId, states);
        if (rootId == -1) return composition;

        // Traverse all entities to find those belonging to the same root
        for (int i = 0; i < (int)states.size(); i++) {
            if (findMoleculeRoot(i, states) == rootId) {
                composition[atoms[i].atomicNumber]++;
            }
        }
        return composition;
    }
}

#endif // MATH_UTILS_HPP


