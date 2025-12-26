#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

/**
 * DATA REPRESENTATION (ECS)
 * Minimum data required for maximum caching and simulation speed.
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

// Clustering State (Molecules)
struct StateComponent {
    bool isClustered;
    int moleculeId;      // -1 if not in a stable molecule
    int parentEntityId;  // Entity this one is anchored to (-1 = root)
    int parentSlotIndex; // Which parent slot it occupies
    float dockingProgress; // 0.0 = recently bonded, 1.0 = fully coupled (animation)
    bool isShielded;     // Blocks spontaneous bonding (used by Tractor Beam)

    // --- OPTIMIZATIONS (Phase 17) ---
    int childCount;      // Number of atoms bonded to this one (as children)
    unsigned char occupiedSlots; // Bitset (1 bit per slot index) to find free slots in O(1)
    
    // --- PHASE 18: CYCLES & MEMBRANES ---
    int cycleBondId;     // Entity ID for closing a loop (Non-hierarchical bond)
};


#endif
