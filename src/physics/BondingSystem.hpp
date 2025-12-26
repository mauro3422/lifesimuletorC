#ifndef BONDING_SYSTEM_HPP
#define BONDING_SYSTEM_HPP

#include "../ecs/components.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include <vector>

class EnvironmentManager;
class SpatialGrid;

/**
 * BONDING SYSTEM (Deterministic)
 * Handles atom bonding into VSEPR slots and updating rigid molecular hierarchies.
 */
class BondingSystem {
public:
    enum BondError {
        SUCCESS = 0,
        VALENCY_FULL,
        DISTANCE_TOO_FAR,
        ANGLE_INCOMPATIBLE,
        ALREADY_CLUSTERED,
        INTERNAL_ERROR
    };

    // Attempts to bond one entity to another in a free slot
    // If forced is true, ignores angle threshold (player-initiated)
    static BondError tryBond(int sourceId, int targetId, 
                       std::vector<StateComponent>& states,
                       std::vector<AtomComponent>& atoms,
                       const std::vector<TransformComponent>& transforms,
                       bool forced = false,
                       float angleMultiplier = 1.0f);

    // Updates child positions based on hierarchy
    static void updateHierarchy(std::vector<TransformComponent>& transforms,
                               std::vector<StateComponent>& states,
                               const std::vector<AtomComponent>& atoms);

    // Autonomous molecular evolution: Allows NPCs to bond spontaneously
    // Uses SpatialGrid for O(N*k) performance instead of O(N²)
    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         std::vector<AtomComponent>& atoms,
                                         const std::vector<TransformComponent>& transforms,
                                         const class SpatialGrid& grid,
                                         EnvironmentManager* env = nullptr,
                                         int tractedEntityId = -1);

    // Cleanly breaks a bond (reverts polarity and releases atom)
    static void breakBond(int entityId, std::vector<StateComponent>& states, 
                          std::vector<AtomComponent>& atoms);

    // Fully isolates an atom by breaking all connections (parent and children)
    static void breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                               std::vector<AtomComponent>& atoms);

    // Finds the last direct child of a parent (highest entity index)
    static int findLastChild(int parentId, const std::vector<StateComponent>& states);

    // Finds a terminal atom (leaf) in a parent's hierarchy
    static int findPrunableLeaf(int parentId, const std::vector<StateComponent>& states);

    // Returns the first free slot index (used for magnetic docking)
    static int getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states,
                                const std::vector<AtomComponent>& atoms);

private:
    // Returns the index of the free slot closest to a relative position
    static int getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle = false,
                                   float angleMultiplier = 1.0f);
};

#endif
