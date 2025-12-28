# 🔨 Build Instructions

## Prerequisites

- **Compiler**: MinGW-w64 (g++ with C++17 support)
- **Graphics**: Raylib 5.0 (included in `external/`)
- **OS**: Windows 10/11

## Quick Build

```powershell
# From the project root (LifeSimulatorCPP/)
./build.ps1
```

This will:
1. Compile all source files
2. Link against Raylib
3. Generate `LifeSimulator.exe`
4. Auto-run the executable if successful

## Manual Build

```powershell
g++ src/main.cpp `
    src/core/LocalizationManager.cpp `
    src/core/JsonLoader.cpp `
    src/physics/PhysicsEngine.cpp `
    src/physics/StructuralPhysics.cpp `
    src/physics/SpatialGrid.cpp `
    src/physics/BondingSystem.cpp `
    src/rendering/Renderer25D.cpp `
    src/input/InputHandler.cpp `
    src/chemistry/ChemistryDatabase.cpp `
    src/chemistry/StructureRegistry.cpp `
    src/gameplay/Player.cpp `
    src/gameplay/TractorBeam.cpp `
    src/ui/LabelSystem.cpp `
    src/ui/Inspector.cpp `
    src/ui/HUD.cpp `
    src/ui/UIWidgets.cpp `
    src/ui/Quimidex.cpp `
    src/gameplay/MissionManager.cpp `
    -I"external/raylib/raylib-5.0_win64_mingw-w64/include" `
    -I"src" `
    -L"external/raylib/raylib-5.0_win64_mingw-w64/lib" `
    -lraylib -lopengl32 -lgdi32 -lwinmm `
    -static-libgcc -static-libstdc++ `
    -O2 -Wall -std=c++17 -pthread `
    -o LifeSimulator.exe
```

## Running Tests

```powershell
./run_tests.ps1
```

This runs:
- **Integration Tests**: Ring topology, ladder diagnostics
- **Standalone Tests**: VSEPR geometry validation (50+ assertions)

## Project Structure

```
LifeSimulatorCPP/
├── src/                    # Source code
│   ├── main.cpp           # Entry point
│   ├── core/              # Config, MathUtils, Localization
│   ├── ecs/               # Entity Component System
│   ├── physics/           # PhysicsEngine, BondingSystem
│   ├── chemistry/         # ChemistryDatabase, Elements
│   ├── gameplay/          # Player, TractorBeam, Missions
│   ├── rendering/         # 2.5D Renderer, Camera
│   ├── ui/                # Inspector, HUD, Quimidex
│   └── world/             # Zones (ClayZone)
├── data/                   # JSON data files
│   ├── elements.json      # Periodic table (CHNOPS)
│   ├── molecules.json     # Known molecules
│   ├── structures.json    # Ring definitions
│   └── lang_*.json        # Localization
├── tests/                  # Test files
├── docs/                   # Documentation
├── external/               # Raylib library
└── build.ps1              # Build script
```

## Troubleshooting

### "g++ not found"
Install MinGW-w64 and add to PATH.

### "raylib.h not found"
Ensure `external/raylib/raylib-5.0_win64_mingw-w64/` exists.

### Tests fail with Raylib conflict
Known issue: Doctest + Raylib headers have macro conflicts (`near`, `far`, `Rectangle`). 
Integration tests use custom assertion macros to avoid this.

---

*See also: [ARCHITECTURE.md](ARCHITECTURE.md) for system overview*
