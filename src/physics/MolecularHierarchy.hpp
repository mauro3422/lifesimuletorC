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
        return MathUtils::findMoleculeRoot(entityId, states);
    }

    static void propagateMoleculeId(int entityId, int newMoleculeId, std::vector<StateComponent>& states) {
        if (entityId < 0 || entityId >= (int)states.size()) return;

        // Prevent redundant recursion
        if (states[entityId].moleculeId == newMoleculeId) return;

        states[entityId].moleculeId = newMoleculeId;

        // 1. Propagate to children (standard hierarchy)
        for (int i = 0; i < (int)states.size(); i++) {
            if (states[i].parentEntityId == entityId) {
                propagateMoleculeId(i, newMoleculeId, states);
            }
        }

        // 2. Propagate across cycle bonds (non-hierarchical)
        if (states[entityId].cycleBondId != -1) {
            propagateMoleculeId(states[entityId].cycleBondId, newMoleculeId, states);
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
