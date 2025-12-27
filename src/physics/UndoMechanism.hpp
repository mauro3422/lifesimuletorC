#ifndef UNDO_MECHANISM_HPP
#define UNDO_MECHANISM_HPP

#include <vector>
#include "../ecs/components.hpp"

/**
 * UndoMechanism (Phase 30)
 * Utilities for finding atoms to prune during undo operations.
 */
class UndoMechanism {
public:
    static int findLastChild(int parentId, const std::vector<StateComponent>& states) {
        int lastChild = -1;
        for (int i = 0; i < (int)states.size(); i++) {
            if (states[i].parentEntityId == parentId) {
                if (lastChild == -1 || i > lastChild) {
                    lastChild = i;
                }
            }
        }
        return lastChild;
    }

    static int findPrunableLeaf(int parentId, const std::vector<StateComponent>& states) {
        // Find a child that is itself a leaf (has no children)
        for (int i = (int)states.size() - 1; i >= 0; i--) {
            if (states[i].parentEntityId == parentId) {
                // Check if this child has children
                bool hasChildren = false;
                for (int j = 0; j < (int)states.size(); j++) {
                    if (states[j].parentEntityId == i) {
                        hasChildren = true;
                        break;
                    }
                }
                if (!hasChildren) return i;
            }
        }
        return -1;
    }
};

#endif // UNDO_MECHANISM_HPP
