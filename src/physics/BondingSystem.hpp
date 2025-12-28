#ifndef BONDING_SYSTEM_HPP
#define BONDING_SYSTEM_HPP

#include "../ecs/components.hpp"
#include "BondingTypes.hpp"
#include "raylib.h" 
#include <vector>

// Forward Declarations
class EnvironmentManager;
class SpatialGrid;
struct Element;

/**
 * BONDING SYSTEM (Facade)
 * Deterministic interface for atom bonding, delegating to specialized modules.
 * Fully decoupled from implementation details in the header.
 */
class BondingSystem {
public:
    // Constants mapping to shared enum for backward compatibility/ease of use
    static constexpr BondError SUCCESS = BondError::SUCCESS;
    static constexpr BondError VALENCY_FULL = BondError::VALENCY_FULL;
    static constexpr BondError DISTANCE_TOO_FAR = BondError::DISTANCE_TOO_FAR;
    static constexpr BondError ANGLE_INCOMPATIBLE = BondError::ANGLE_INCOMPATIBLE;
    static constexpr BondError ALREADY_CLUSTERED = BondError::ALREADY_CLUSTERED;
    static constexpr BondError ALREADY_BONDED = BondError::ALREADY_BONDED;
    static constexpr BondError INTERNAL_ERROR = BondError::INTERNAL_ERROR;

    // --- Facade Methods (Implemented in .cpp) ---

    static bool canAcceptBond(int entityId, const std::vector<StateComponent>& states, const Element& element);

    static BondError tryBond(int sourceId, int targetId, 
                       std::vector<StateComponent>& states,
                       std::vector<AtomComponent>& atoms,
                       const std::vector<TransformComponent>& transforms,
                       bool forced = false,
                       float angleMultiplier = 1.0f);

    static void updateHierarchy(std::vector<TransformComponent>& transforms,
                               std::vector<StateComponent>& states,
                               const std::vector<AtomComponent>& atoms);

    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         std::vector<AtomComponent>& atoms,
                                         std::vector<TransformComponent>& transforms,
                                         const SpatialGrid& grid,
                                         const std::vector<int>& rootCache,
                                         EnvironmentManager* env = nullptr,
                                         int tractedEntityId = -1);

    static void breakBond(int entityId, std::vector<StateComponent>& states, 
                          std::vector<AtomComponent>& atoms);

    static void breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                               std::vector<AtomComponent>& atoms);

    static int findLastChild(int parentId, const std::vector<StateComponent>& states);

    static int findPrunableLeaf(int parentId, const std::vector<StateComponent>& states);

    static int getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle = false,
                                   float angleMultiplier = 1.0f);

    static int getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states,
                                const std::vector<AtomComponent>& atoms);

    static BondError tryCycleBond(int i, int j, std::vector<StateComponent>& states, 
                                 std::vector<AtomComponent>& atoms, 
                                 std::vector<TransformComponent>& transforms);

    static void propagateMoleculeId(int entityId, int newMoleculeId, std::vector<StateComponent>& states);
};

#endif
