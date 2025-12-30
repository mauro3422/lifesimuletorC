#ifndef MOLECULAR_HIERARCHY_HPP
#define MOLECULAR_HIERARCHY_HPP

#include <vector>
#include "../ecs/components.hpp"
#include "../core/MathUtils.hpp"

/**
 * MolecularHierarchy (Phase 30)
 * Handles tree-structure traversal and root synchronization for molecules.
 */
class MolecularHierarchy {
public:
    static int findRoot(int entityId, const std::vector<StateComponent>& states) {
        if (entityId < 0 || entityId >= (int)states.size()) return -1;
        // The cached moleculeId IS the root index in our unified system.
        return (states[entityId].moleculeId != -1) ? states[entityId].moleculeId : entityId;
    }

    /**
     * Propagates a moleculeId across the entire connected cluster (parents, children, and cycles).
     * Uses iterative BFS to prevent stack overflow in large networks (Phase 42).
     */
    static void propagateMoleculeId(int seedEntityId, std::vector<StateComponent>& states) {
        if (seedEntityId < 0 || seedEntityId >= (int)states.size()) return;

        // 1. Find all members of the cluster
        std::vector<int> members;
        std::vector<bool> visited(states.size(), false);
        std::vector<int> stack = { seedEntityId };
        visited[seedEntityId] = true;

        int minId = seedEntityId;

        size_t head = 0;
        members.push_back(seedEntityId);

        while (head < members.size()) {
            int curr = members[head++];
            if (curr < minId) minId = curr;

            // a. Check Parent
            int p = states[curr].parentEntityId;
            if (p != -1 && !visited[p]) {
                visited[p] = true;
                members.push_back(p);
            }

            // b. Check Children (Optimization: we scan the whole list, in Phase 43 we should add child lists)
            for (int i = 0; i < (int)states.size(); i++) {
                if (states[i].parentEntityId == curr && !visited[i]) {
                    visited[i] = true;
                    members.push_back(i);
                }
            }

            // c. Check Cycle Bond
            int c = states[curr].cycleBondId;
            if (c != -1 && !visited[c]) {
                visited[c] = true;
                members.push_back(c);
            }
        }

        // 2. Apply the deterministic root ID (lowest index in cluster)
        // and update isClustered flag based on cluster size (Phase 42 Fix)
        bool hasConnections = (members.size() > 1);
        for (int idx : members) {
            states[idx].moleculeId = minId;
            states[idx].isClustered = hasConnections;
        }
    }

    static std::vector<int> getChildren(int parentId, const std::vector<StateComponent>& states) {
        std::vector<int> children;
        for (int i = 0; i < (int)states.size(); i++) {
            if (states[i].parentEntityId == parentId) {
                children.push_back(i);
            }
        }
        return children;
    }
};

#endif // MOLECULAR_HIERARCHY_HPP
