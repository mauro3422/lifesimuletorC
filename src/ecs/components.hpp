#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <cstdint>

/**
 * DATA REPRESENTATION (ECS)
 * Minimum data required for maximum caching and simulation speed.
 * 
 * Phase 31: Fields documented by logical grouping.
 * All structs are aggregates (no user-defined constructors) for optimal push_back performance.
 */

// Position, Velocity, and Rotation
struct TransformComponent {
    float x, y, z;
    float vx, vy, vz;
    float rotation;
};

// Atom Data
struct AtomComponent {
    int atomicNumber;  // Unique ID (Z). Detailed properties fetched from ChemistryDatabase.
    float partialCharge;
};

/**
 * STATE COMPONENT (Clustered State for Molecules)
 * 
 * Fields are organized into logical groups for clarity:
 * 1. Hierarchy: Core bonding tree structure
 * 2. Ring: Cycle/membrane data (Phase 18+)
 * 3. Physics: Animation and temporary flags
 * 
 * This is an AGGREGATE - no user-defined constructors for optimal vector performance.
 * Use designated initializers or brace initialization: StateComponent{.isClustered=true}
 */
struct StateComponent {
    // === HIERARCHY GROUP ===
    bool isClustered = false;
    int moleculeId = -1;
    int parentEntityId = -1;
    int parentSlotIndex = -1;
    float dockingProgress = 1.0f;  // Original position for backward compatibility
    bool isShielded = false;
    int childCount = 0;
    uint32_t occupiedSlots = 0;

    // === RING GROUP ===
    int cycleBondId = -1;
    bool isInRing = false;
    int ringSize = 0;
    int ringIndex = -1;
    int ringInstanceId = -1;
    float targetX = 0.0f;  // Absolute target position for docking animation
    float targetY = 0.0f;

    // === PHYSICS GROUP ===
    bool justBonded = false;
    float releaseTimer = 0.0f; // Time since isShielded was set back to false

    // === UTILITY METHODS ===
    bool isLocked() const { return isClustered && dockingProgress >= 0.99f && !isShielded; }
};

#endif
