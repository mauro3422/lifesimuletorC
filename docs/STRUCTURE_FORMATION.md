# Structure Formation System - Technical Reference

## Overview
This document captures the learnings and patterns discovered while implementing the 4-carbon ring (cyclobutane) formation system. Use this as a reference for implementing other molecular structures.

## Core Pattern for Stable Structures

```
1. DETECT completion trigger (e.g., cycle closure, pattern match)
2. COLLECT all participating atoms in ORDER
3. CALCULATE ideal positions based on geometry
4. REPOSITION atoms instantly to target positions
5. RESET velocities to zero
6. SET structure flag (e.g., isInRing)
7. EXCLUDE flagged atoms from dynamic forces
8. APPLY heavy damping to maintain shape
```

## Key Files Modified

| File | Purpose |
|------|---------|
| `BondingSystem.cpp` | Detection, atom collection, repositioning |
| `PhysicsEngine.cpp` | Force exclusion, damping, stability |
| `Renderer25D.cpp` | Visual feedback (highlight, vibration) |
| `components.hpp` | State flags (`isInRing`, `cycleBondId`) |

## Critical Learnings

### 1. Angular Forces Cause Oscillation
**Problem**: Active angular correction forces + spring forces = feedback loop oscillation.
**Solution**: Don't use active forces to maintain shape. Position correctly at formation, then freeze.

### 2. Use isInRing, Not cycleBondId
**Problem**: Only 2 atoms have `cycleBondId` set, but all 4 need protection.
**Solution**: Use `isInRing` flag for all atoms in the structure.

### 3. Chain Order Matters for Bonds
**Problem**: Random atom order → bonds connect wrong corners.
**Solution**: Walk the parent chain to collect atoms in sequence order.

### 4. Clean Up All Related Atoms
**Problem**: Breaking one bond left other atoms with stale flags.
**Solution**: When any structural bond breaks, clear flags for ALL connected atoms.

## Configuration Constants

```cpp
// In PhysicsEngine.cpp - Ring stability
float ringDamping = 0.30f;     // Heavy damping (lower = more frozen)
float zFlattenForce = 20.0f;   // Keep ring flat
float ringSpringK = 6.0f;      // Gentle distance correction

// In Renderer25D.cpp - Visual vibration
float vibSpeed = 0.08f;        // Oscillation speed
float vibAmplitude = 0.6f;     // Pixels of movement
```

## Future Generalization: Structure Definition System

For new structures, we need:

```cpp
struct StructureDefinition {
    int atomCount;              // e.g., 4 for square, 5 for pentagon
    float targetAngle;          // Internal angle (90° for square, 108° for pentagon)
    std::vector<Vector2> offsets; // Relative positions from centroid
    bool isPlanar;              // Force Z=0?
    float damping;              // Stability (0.3 = frozen, 0.9 = flexible)
};
```

## Debug Flags (TODO)
Add to `Config.hpp` for testing:
```cpp
static constexpr bool DEBUG_INSTANT_FORMATION = true;  // Skip animation
static constexpr bool DEBUG_DISABLE_RING_DAMPING = false;
static constexpr bool DEBUG_LOG_RING_FORCES = false;
```

## Test Procedure
1. Navigate to Clay Island (-1200, -400)
2. Drop 4 Carbon atoms nearby
3. Observe automatic chain formation
4. Wait for cycle closure
5. Verify square geometry and stability
6. Test disconnection → all flags clear
