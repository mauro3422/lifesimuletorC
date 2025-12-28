# 🏗️ Architecture Overview

## System Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         MAIN LOOP                                │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐     │
│  │  Input   │──▶│ Physics  │──▶│ Bonding  │──▶│ Render   │     │
│  │ Handler  │   │  Engine  │   │  System  │   │  2.5D    │     │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘     │
└─────────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
        ┌──────────┐   ┌──────────┐   ┌──────────┐
        │   ECS    │   │Chemistry │   │   UI     │
        │  World   │   │ Database │   │ Widgets  │
        └──────────┘   └──────────┘   └──────────┘
```

## Core Architecture: ECS

The project uses a **Component-Based Entity System**:

```cpp
// Entity = Index (int)
// Components = Data structs

struct TransformComponent {
    float x, y, z;      // Position
    float vx, vy, vz;   // Velocity
};

struct AtomComponent {
    int atomicNumber;
    float partialCharge;
};

struct StateComponent {
    bool isClustered;
    int parentEntityId;
    int parentSlotIndex;
    bool isInRing;
    int cycleBondId;
    // ... more flags
};
```

## Module Responsibilities

### Physics Layer (`src/physics/`)

| Module | Responsibility |
|--------|---------------|
| `PhysicsEngine` | Coulomb forces, springs, integration |
| `BondingSystem` | Hierarchy management, bond creation |
| `BondingCore` | Slot validation, valency checks |
| `RingChemistry` | Cycle detection, LCA calculation |
| `AutonomousBonding` | Spontaneous bonding rules |
| `StructuralPhysics` | Ring dynamics, folding |
| `SpatialGrid` | O(1) neighbor queries |

### Chemistry Layer (`src/chemistry/`)

| Module | Responsibility |
|--------|---------------|
| `ChemistryDatabase` | Element/molecule lookup |
| `StructureRegistry` | Ring definitions from JSON |
| `Element` | Atomic properties struct |

### Gameplay Layer (`src/gameplay/`)

| Module | Responsibility |
|--------|---------------|
| `Player` | Movement, molecule control |
| `TractorBeam` | Atom capture and transport |
| `DockingSystem` | Auto-docking animation |
| `UndoManager` | Hierarchical undo stack |
| `MissionManager` | Quest/objective tracking |

### UI Layer (`src/ui/`)

| Module | Responsibility |
|--------|---------------|
| `Inspector` | Atom/molecule detail panel |
| `Quimidex` | Educational molecule catalog |
| `HUD` | Status bar, zoom indicator |
| `NotificationManager` | Toast messages |
| `UIWidgets` | Reusable panel components |
| `LabelSystem` | Floating atom labels |

## Data Flow

```
1. INPUT
   └─▶ InputHandler captures keys/mouse
   
2. SIMULATION (Fixed 60Hz)
   ├─▶ Player.update() - Movement & tractor
   ├─▶ PhysicsEngine.step()
   │   ├─▶ Coulomb forces
   │   ├─▶ Spring forces (bonds)
   │   ├─▶ StructuralPhysics (rings)
   │   └─▶ Integration + friction
   └─▶ BondingSystem.updateHierarchy()
   
3. RENDER (VSync)
   ├─▶ Environment zones
   ├─▶ Renderer25D.drawAtoms()
   ├─▶ LabelSystem.draw()
   └─▶ UI panels (Inspector, HUD, Quimidex)
```

## Key Design Patterns

### Facade Pattern
`BondingSystem` acts as a facade delegating to:
- `BondingCore` (atomic operations)
- `RingChemistry` (cycles)
- `MolecularHierarchy` (tree traversal)

### Singleton Pattern
Used for global services:
- `ChemistryDatabase::getInstance()`
- `LocalizationManager::getInstance()`
- `NotificationManager::getInstance()`

### Data-Driven Design
All chemical data in JSON files:
- `data/elements.json` - Periodic table
- `data/molecules.json` - Known compounds
- `data/structures.json` - Ring parameters

## Performance Optimizations

| Optimization | Location | Impact |
|--------------|----------|--------|
| Spatial Grid | `SpatialGrid.cpp` | O(N²) → O(N) |
| Bitmask Slots | `StateComponent` | O(k) → O(1) |
| Root Cache | `PhysicsEngine` | O(N×depth) → O(1) |
| Fixed Timestep | `main.cpp` | Deterministic physics |
| Bonding Throttle | `Config.hpp` | 60Hz → 10Hz |

---

*See also: [BUILDING.md](BUILDING.md) for compilation instructions*
