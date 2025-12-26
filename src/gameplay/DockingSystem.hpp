#ifndef DOCKING_SYSTEM_HPP
#define DOCKING_SYSTEM_HPP

#include "../ecs/components.hpp"
#include "../physics/BondingSystem.hpp"
#include "../core/MathUtils.hpp"
#include "../core/Config.hpp"
#include "../ui/NotificationManager.hpp"
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

        float dx = transforms[playerIdx].x - transforms[targetIdx].x;
        float dy = transforms[playerIdx].y - transforms[targetIdx].y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < Config::TRACTOR_DOCKING_RANGE * 1.2f) {
            if (BondingSystem::tryBond(targetIdx, playerIdx, states, atoms, transforms, true) 
                == BondingSystem::SUCCESS) {
                int root = MathUtils::findMoleculeRoot(targetIdx, states);
                states[root].isShielded = false;
                attachmentOrder.push_back(targetIdx);
                NotificationManager::getInstance().show("¡Acoplado!", Config::THEME_SUCCESS);
                return true;
            }
        }
        return false;
    }

} // namespace DockingSystem

#endif // DOCKING_SYSTEM_HPP
