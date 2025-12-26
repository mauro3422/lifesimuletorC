#ifndef DOCKING_SYSTEM_HPP
#define DOCKING_SYSTEM_HPP

#include "../ecs/components.hpp"
#include "../physics/BondingSystem.hpp"
#include "../core/MathUtils.hpp"
#include "../core/Config.hpp"
#include "../ui/NotificationManager.hpp"
#include "../core/LocalizationManager.hpp"
#include <vector>
#include <cmath>

/**
 * DOCKING SYSTEM
 * Handles automatic docking when atoms are in range.
 * Extracted from Player.cpp for single-responsibility.
 */
namespace DockingSystem {

    /**
     * Attempt to auto-dock a target atom to the player's molecule.
     * @param targetIdx Index of atom to dock
     * @param playerIdx Player's entity index
     * @param states State components
     * @param atoms Atom components
     * @param transforms Transform components
     * @param attachmentOrder History stack for undo (will be modified)
     * @return true if docking succeeded
     */
    inline bool tryAutoDock(
        int targetIdx,
        int playerIdx,
        std::vector<StateComponent>& states,
        std::vector<AtomComponent>& atoms,
        std::vector<TransformComponent>& transforms,
        std::vector<int>& attachmentOrder
    ) {
        if (targetIdx == -1 || targetIdx == playerIdx) return false;
        if (states[targetIdx].isClustered) return false;

        float distSq = MathUtils::distSq(transforms[playerIdx].x, transforms[playerIdx].y, transforms[targetIdx].x, transforms[targetIdx].y);
        float threshold = Config::TRACTOR_DOCKING_RANGE * 1.2f;

        if (distSq < threshold * threshold) {
            if (BondingSystem::tryBond(targetIdx, playerIdx, states, atoms, transforms, true) 
                == BondingSystem::SUCCESS) {
                int root = MathUtils::findMoleculeRoot(targetIdx, states);
                states[root].isShielded = false;
                attachmentOrder.push_back(targetIdx);
                NotificationManager::getInstance().show(LocalizationManager::getInstance().get("ui.notification.docked"), Config::THEME_SUCCESS);
                return true;
            }
        }
        return false;
    }

} // namespace DockingSystem

#endif // DOCKING_SYSTEM_HPP
