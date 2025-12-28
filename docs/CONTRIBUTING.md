# 🤝 Contributing Guide

## Getting Started

1. **Clone** the repository
2. **Read** [BUILDING.md](BUILDING.md) to set up your environment
3. **Read** [ARCHITECTURE.md](ARCHITECTURE.md) to understand the codebase
4. **Run** `./build.ps1` to verify everything compiles

---

## Code Standards

### Language
- **Code**: English (variable names, function names, comments)
- **User-Facing Strings**: Bilingual (ES/EN) via `LocalizationManager`

### Formatting
- **Indentation**: 4 spaces
- **Braces**: Same line for control structures
- **Line length**: ~120 characters max

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `PhysicsEngine` |
| Functions | camelCase | `updateHierarchy()` |
| Variables | camelCase | `parentEntityId` |
| Constants | UPPER_SNAKE | `BOND_SPRING_K` |
| Files | PascalCase | `BondingSystem.cpp` |

---

## Project Structure

```
src/
├── core/       # Utilities (Config, MathUtils, Localization)
├── ecs/        # Entity Component System
├── physics/    # Simulation engine
├── chemistry/  # Element/molecule database
├── gameplay/   # Player, tractor beam, missions
├── rendering/  # 2.5D graphics
├── ui/         # Panels, widgets, HUD
└── world/      # Environment zones
```

---

## Adding New Features

### New Element
1. Add entry to `data/elements.json`
2. Include: atomicNumber, symbol, name (ES/EN), color, bondingSlots
3. Ensure Z-variance in bondingSlots for 3D geometry

### New Molecule
1. Add entry to `data/molecules.json`
2. Include: id, formula, composition, name, description, origin

### New UI Panel
1. Create `.hpp` and `.cpp` in `src/ui/`
2. Follow `UIWidgets` protocol (see [UI_STANDARDS.md](UI_STANDARDS.md))
3. Use `InputHandler` for mouse capture
4. Add to `build.ps1`

### New Physics Feature
1. Consider if it belongs in `PhysicsEngine` or a specialized module
2. Use `Config::` constants for all magic numbers
3. Add tests to verify behavior

---

## Localization

All user-facing strings must be localized:

```cpp
// ❌ Wrong
DrawText("Health:", x, y, 12, WHITE);

// ✅ Correct
DrawText(lang.get("ui.hud.health").c_str(), x, y, 12, WHITE);
```

Add keys to both `data/lang_es.json` and `data/lang_en.json`.

---

## Testing

- Run `./run_tests.ps1` before committing
- Add tests for new bonding/physics logic
- See [TESTING.md](TESTING.md) for details

---

## Commit Messages

Use descriptive messages:
```
[Physics] Add stress-based bond breaking for NPC molecules
[UI] Implement Quimidex molecule catalog
[Fix] Resolve ghost membrane rendering bug
[Docs] Update ROADMAP with completed phases
```

---

*Thank you for contributing! 🎮🧬*
