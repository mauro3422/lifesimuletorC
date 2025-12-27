#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include "raylib.h"
#include <vector>
#include <map>
#include <cmath>
#include "../ecs/components.hpp"

namespace MathUtils {

    /**
     * ATOM PAIR (Phase 28 Optimization)
     * Encapsulates two entities and allows for lazy evaluation of their distance.
     */
    struct AtomPair {
        int i, j;
        mutable float cachedDistSq = -1.0f;
        mutable float cachedDist = -1.0f;

        float distSq(const std::vector<TransformComponent>& tr) const {
            if (cachedDistSq < 0) {
                float dx = tr[j].x - tr[i].x;
                float dy = tr[j].y - tr[i].y;
                float dz = tr[j].z - tr[i].z;
                cachedDistSq = dx*dx + dy*dy + dz*dz;
            }
            return cachedDistSq;
        }

        float dist(const std::vector<TransformComponent>& tr) const {
            if (cachedDist < 0) {
                cachedDist = std::sqrt(distSq(tr));
            }
            return cachedDist;
        }
    };

    // Generates a random jitter between -1.0 and 1.0
    inline float getJitter() {
        return (float)GetRandomValue(-100, 100) / 100.0f;
    }

    // --- VECTOR MATH (Centralized) ---
    inline float distSq(float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return dx*dx + dy*dy;
    }

    inline float distSq(Vector2 v1, Vector2 v2) {
        return distSq(v1.x, v1.y, v2.x, v2.y);
    }

    inline float dist(float x1, float y1, float x2, float y2) {
        return std::sqrt(distSq(x1, y1, x2, y2));
    }

    inline float dist(Vector2 v1, Vector2 v2) {
        return dist(v1.x, v1.y, v2.x, v2.y);
    }

    inline float dist(const TransformComponent& t1, const TransformComponent& t2) {
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float dz = t2.z - t1.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    inline float length(float x, float y, float z = 0.0f) {
        return std::sqrt(x*x + y*y + z*z);
    }

    inline float length(Vector2 v) {
        return std::sqrt(v.x*v.x + v.y*v.y);
    }

    inline Vector3 normalize(float x, float y, float z) {
        float len = length(x, y, z);
        if (len < 0.0001f) return {0, 0, 0};
        return {x/len, y/len, z/len};
    }

    inline Vector3 normalize(Vector3 v) {
        return normalize(v.x, v.y, v.z);
    }

    inline Vector2 normalize(Vector2 v) {
        float len = length(v);
        if (len < 0.0001f) return {0, 0};
        return {v.x/len, v.y/len};
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

    /**
     * Identifies all atom indices belonging to the same molecular structure.
     * Uses the pre-propagated moleculeId for O(1) comparison during the O(N) scan.
     */
    inline std::vector<int> getMoleculeMembers(int entityId, const std::vector<StateComponent>& states) {
        std::vector<int> members;
        if (entityId < 0 || entityId >= (int)states.size()) return members;

        // Current root ID (either stored in moleculeId or is itself)
        int rootId = (states[entityId].moleculeId == -1) ? entityId : states[entityId].moleculeId;

        for (int i = 0; i < (int)states.size(); i++) {
            if (states[i].moleculeId == rootId || i == rootId) {
                members.push_back(i);
            }
        }
        return members;
    }

    /**
     * Scans a molecular cluster and returns its atomic composition (AtomicNumber -> Count).
     */
    inline std::map<int, int> getMoleculeComposition(int entityId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
        std::map<int, int> composition;
        std::vector<int> members = getMoleculeMembers(entityId, states);
        for (int idx : members) {
            composition[atoms[idx].atomicNumber]++;
        }
        return composition;
    }

}

#endif // MATH_UTILS_HPP


