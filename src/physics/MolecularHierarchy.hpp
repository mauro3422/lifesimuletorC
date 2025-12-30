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
    // findRoot is now unified in MathUtils::findMoleculeRoot() (Phase 43)

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

            // b. Check Children using childList (Phase 43 optimization: O(k) instead of O(N))
            for (int childId : states[curr].childList) {
                if (!visited[childId]) {
                    visited[childId] = true;
                    members.push_back(childId);
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

    // getChildren is now O(1) via states[parentId].childList (Phase 43)
    static const std::vector<int>& getChildren(int parentId, const std::vector<StateComponent>& states) {
        static const std::vector<int> empty;
        if (parentId < 0 || parentId >= (int)states.size()) return empty;
        return states[parentId].childList;
    }
};

#endif // MOLECULAR_HIERARCHY_HPP
