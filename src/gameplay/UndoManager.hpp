#ifndef UNDO_MANAGER_HPP
#define UNDO_MANAGER_HPP

#include "../ecs/components.hpp"
#include "../physics/BondingSystem.hpp"
#include "../core/MathUtils.hpp"
#include "../ui/NotificationManager.hpp"
#include "../core/Config.hpp"
#include "../core/LocalizationManager.hpp"
#include <vector>

/**
 * UNDO MANAGER
 * Manages attachment history for hierarchical undo.
 * Extracted from Player.cpp for single-responsibility.
 */
class UndoManager {
public:
    /**
     * Record an attachment for future undo.
     */
    void recordAttachment(int atomId) {
        attachmentOrder.push_back(atomId);
    }

    /**
     * Attempt to undo the last attachment.
     * @param playerIdx Player's entity index
     * @param states State components
     * @param atoms Atom components
     * @return true if an undo was performed
     */
    bool undoLast(
        int playerIdx,
        std::vector<StateComponent>& states,
        std::vector<AtomComponent>& atoms
    ) {
        auto& lm = LocalizationManager::getInstance();

        // First check if player itself is attached to something
        if (states[playerIdx].parentEntityId != -1) {
            BondingSystem::breakBond(playerIdx, states, atoms);
            NotificationManager::getInstance().show(lm.get("ui.notification.player_released"), Config::THEME_INFO);
            return true;
        }

        // Try to undo from attachment history
        while (!attachmentOrder.empty()) {
            int candidate = attachmentOrder.back();
            attachmentOrder.pop_back();
            
            if (states[candidate].isClustered && states[candidate].parentEntityId != -1) {
                if (MathUtils::findMoleculeRoot(candidate, states) == playerIdx) {
                    BondingSystem::breakBond(candidate, states, atoms);
                    NotificationManager::getInstance().show(lm.get("ui.notification.atom_released"), Config::THEME_INFO);
                    return true;
                }
            }
        }

        // Fallback: prune any leaf
        int leafId = BondingSystem::findPrunableLeaf(playerIdx, states);
        if (leafId != -1) {
            BondingSystem::breakBond(leafId, states, atoms);
            NotificationManager::getInstance().show(lm.get("ui.notification.leaf_pruned"), Config::THEME_INFO);
            return true;
        }

        return false;
    }

    /**
     * Clear all attachment history.
     */
    void clear() {
        attachmentOrder.clear();
    }

    /**
     * Get reference to attachment order for DockingSystem.
     */
    std::vector<int>& getAttachmentOrder() {
        return attachmentOrder;
    }

private:
    std::vector<int> attachmentOrder;
};

#endif // UNDO_MANAGER_HPP
