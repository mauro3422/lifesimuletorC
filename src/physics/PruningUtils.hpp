#ifndef PRUNING_UTILS_HPP
#define PRUNING_UTILS_HPP

#include <vector>
#include "../ecs/components.hpp"

/**
 * PruningUtils (Phase 30)
 * Low-level utilities for finding atoms to prune during undo operations.
 * Provides tree traversal helpers for the molecular hierarchy.
 * 
 * NOTE: Renamed from UndoMechanism to clarify its distinction from 
 * gameplay/UndoManager which handles player attachment history.
 */
class PruningUtils {
public:
    /**
     * Finds the most recently added child of a parent atom.
     * @param parentId The parent atom's entity ID
     * @param states State components vector
     * @return Entity ID of the last child, or -1 if none found
     */
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

    /**
     * Finds a leaf node (atom with no children) attached to the parent.
     * Searches in reverse order to find the most recently added leaf first.
     * @param parentId The parent atom's entity ID
     * @param states State components vector
     * @return Entity ID of a prunable leaf, or -1 if none found
     */
    static int findPrunableLeaf(int parentId, const std::vector<StateComponent>& states) {
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

// Backward compatibility alias (deprecated)
using UndoMechanism = PruningUtils;

#endif // PRUNING_UTILS_HPP
