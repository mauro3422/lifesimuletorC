
## [Phase 25: Structural Symmetry & Normalized Physics] - 2025-12-27

### Added
- **Ring Instance ID (`ringInstanceId`)**: Added unique identification for individual rings within complex molecules.
- **Independent Centroids**: `PhysicsEngine` now calculates centroids per-ring rather than per-molecule, enabling multi-ring stability.
- **Parametric Formation Physics**: Moved all assembly magic numbers to `structures.json`:
    - `formationDamping`: Assembly phase friction.
    - `maxFormationSpeed`: Velocity clamping to prevent explosive assembly.
    - `completionThreshold`: Precision target for rigid locking.
- **Improved Detection**: Increased `BOND_AUTO_RANGE` (50.0 -> 55.0) for more reliable bonding of moving atoms.

### Fixed
- **Structural Asymmetry**: Synchronized formation completion via "Collaborative Handshake" - whole group locks only when everyone is ready.
- **Explosive Transitions**: Replaced direct velocity bias with force-based attraction and relative speed clamping.
- **Centroid Distortion**: Cycle atoms are now correctly identified via LCA, preventing branch atoms from skewing the ring center.
- **Stalled Animation**: Adjusted visual docking progress speed to match the physics-driven attraction.

---

## [Phase 24: Stable Ring Formation & Chain Growth] - 2025-12-27

### Fixed
- **Ring Atom Oscillation**: Removed angular forces that caused feedback loop oscillation. Ring stability now achieved through damping (0.30) instead of active forces.
- **Uneven Ring Movement**: Fixed Phase 32/36 still affecting 2 of 4 ring atoms by changing `cycleBondId != -1` check to `isInRing` check.
- **Bond Hierarchy Order**: Fixed ring atom reordering - now collects atoms in chain order before positioning, so bonds connect adjacent corners.
- **isInRing Cleanup Bug**: Enhanced `breakBond()` to clear isInRing for ALL connected atoms when any ring bond breaks, not just the breaking atom.
- **Carbon Chain Growth**: Removed restrictive "root-with-child" logic that prevented 3rd+ atoms from joining chains.

### Added
- **Instant Square Repositioning**: When 4-atom ring forms, atoms immediately snap to perfect square corners around centroid.
- **Visual Ring Vibration**: Added subtle sin/cos oscillation (0.08 speed, 0.6px amplitude) to ring atom borders for "alive" appearance.
- **Cycle Bond Distance Physics**: Unified cycle bond physics with regular ring bonds (K=6.0, distance-based).

### Changed
- **Ring Damping**: Increased from 0.50 to very heavy 0.30 to nearly freeze ring atoms.
- **Z-Flatten Force**: Increased to 20.0 with 0.5 damping for flat 2D rings.
- `updateSpontaneousBonding` and `tryCycleBond` now accept non-const transforms for repositioning.

### Technical Learnings
> **Key Discovery**: Angular forces + spring forces create oscillation feedback loops.
> **Solution**: Position atoms correctly at formation time, then use heavy damping to maintain.
> **Generalization**: For any stable structure, the pattern is:
> 1. Detect structure completion (e.g., cycle closure)
> 2. Calculate ideal target positions based on geometry
> 3. Instantly reposition atoms to target
> 4. Apply heavy damping to maintain shape
> 5. Use `isInRing`/`isInStructure` flags to exclude from dynamic forces

---

## [Phase 22: Ring System Bug Fixes] - 2025-12-27

### Fixed
- **Ghost Ring Highlight Bug**: Fixed issue where atoms retained blue ring highlight after cycle bond was broken. Added `cycleBondId` and `isInRing` cleanup in `breakBond()`.
- **Duplicate Code**: Removed duplicate `isClustered = true` line in `tryBond()`.
- **Full Isolation Cycle Cleanup**: Added cycle bond cleanup in `breakAllBonds()` to ensure complete isolation.

### Changed
- **Reduced Terminal Oscillation**: Lowered ring folding strength from 25.0 to 18.0 to prevent visual vibration while still allowing ring closure.

---

## [Phase 23: Ring Angular Forces] - 2025-12-27

### Added
- **Ring Angular Forces**: Implemented angular bending forces for atoms in rings to maintain proper geometry. Rings now form squares (4 atoms) instead of collapsing into a ball.
- **90° Target Angle**: For 4-atom rings, forces push neighbors to maintain ~90° angles at each vertex.
- **Tangential Force Application**: Angular correction is applied as tangential forces to the two neighbors of each ring atom.
- **Distance-Only Forces for Rings**: Ring atoms now use pure distance springs instead of VSEPR directional forces, allowing angular forces to control geometry.
- **Ring-Specific Damping**: Added 0.92 damping factor specifically for ring atoms to reduce oscillation and stabilize shape faster.

### Changed
- Increased angular spring constant from 3.0 to 6.0 for stronger shape correction.

---

## [Phase 18: Proto-Membranes & Cycle Bonds] - 2025-12-26

### Added
- **Cycle Bond System**: Implemented non-hierarchical structural bonds to allow ring formation (membranes) without breaking the ECS tree.
- **Ring Closure Logic**: "Pure Ring Rule" - Any chain of 4+ atoms (including pure Carbon) now spontaneously fuses ends into stable loops if spatially close.
- **Cycle Physics**: `PhysicsEngine` now applies Hooke's Law to cycle bonds, maintaining ring shape dynamically.
- **StateComponent Upgrade**: Added `cycleBondId` to track loop closures.

---

## [Phase 19: Environment & Balance Audit] - 2025-12-26

### Environment Fixes
- **Solved "Clay Black Hole"**: `ClayZone` was swallowing the entire spawn area.
- **Resized Clay Zone**: Reduced from 2000x1600 to 800x800 and moved to `(-1200, -400)`. It is now a distinct island you must travel to.
- **Physics Tuning**: Reduced Clay adsorption force (5.0 -> 1.0) and drag stickiness (0.95 -> 0.98).
- **Spawn Distribution**: Increased `SPAWN_RANGE_XY` from 250 -> 1500 to disperse atoms widely across the void.
- **Membrane Logic**: Relaxed ring-closure rule from 4 to 3 hops. This allows smaller 4-atom chains (Carbon squares) to form stable membranes.

---

## [Phase 20: Thermodynamics & Stability] - 2025-12-26

### Physics Overhaul
- **System Liveliness**: Increased `THERMODYNAMIC_JITTER` (0.5 -> 2.5). Atoms now vibrate and flow naturally, breaking stagnation.
- **Fluid Dynamics**: Reduced `DRAG_COEFFICIENT` (0.99 -> 0.95). Momentum is preserved longer.
- **Structural Integrity**: Increased `BOND_BREAK_STRESS` (35.0 -> 60.0). Chemical bonds are now much stronger and resist the increased chaotic energy.

---

## [Phase 21: Density & Scale Optimization] - 2025-12-26

### World Density
- **Population Explosion**: Increased `INITIAL_ATOM_COUNT` from 1000 -> 2500.
- **Concentrated Spawn**: Reduced `SPAWN_RANGE_XY` from 1500 -> 1200.
- **Result**: The "void" is now filled with a rich chemical soup (approx 4x density increase).

### Visuals
- **Visibility**: Increased `BASE_ATOM_RADIUS` (6.0 -> 7.0) and `PLAYER_VISUAL_SCALE` (1.6 -> 1.8) for better legibility at zoom.

---

## [Phase 17.5: Codebase Hygiene & Architectual Refactor] - 2025-12-26

### Refactored
- **Header Separation**: Extracted implementations from `UIWidgets.hpp` and `JsonLoader.hpp` into their own `.cpp` files to reduce compilation dependency bloat.
- **Math Unification**: Replaced duplicated distance/vector calculations in `PhysicsEngine` and `BondingSystem` with standardized `MathUtils` calls.
- **Build System**: Updated `build.ps1` to compile the new separated source files (`UIWidgets.cpp`, `JsonLoader.cpp`).

### Fixed
- **Physics Regressions**: Addressed variable scope issues in `PhysicsEngine` during the math refactor.
- **Code Duplication**: Eliminated 5+ instances of repeated vector math logic across the codebase.

---

## [Phase 17: Deep Codebase Audit & Optimization] - 2025-12-26

### Added
- **Full UI Localization**: Complete English/Spanish support for HUD, Quimidex, Inspector, and Notifications.
- **Runtime Language Toggle**: **F1 Key** instantly switches language without restarting.
- **O(1) Bonding Logic**: Implemented `occupiedSlots` bitmask for instant slot detection, replacing O(N) loops.
- **Mass-Based Physics**: Integrated `atomicMass` into all force calculations ($F=ma$); heavy atoms now have real inertia.
- **MathUtils Extensions**: Added `dist`, `distSq`, and `normalize` overloads for `Vector2` (Raylib), consolidating vector math.

### Changed
- **Performance Optimization**: `getBestAvailableSlot` and `getFirstFreeSlot` now run in constant time O(1).
- **Code Standardization**: Translated all internal comments and error messages from Spanish to English in `Config.hpp`, `UIWidgets.hpp`, and `InputHandler.cpp`.
- **Gameplay Polish**: Localized `DockingSystem` and `UndoManager` feedback messages.
- **Deprecation**: Removed ad-hoc math functions in `Player.cpp` and `Renderer25D.cpp` in favor of `MathUtils`.

### Fixed
- **Ambiguous Overloads**: Resolved `MathUtils::normalize` conflicts by using explicit `Vector2{}` casting.
- **Hardcoded Strings**: Eliminated remaining Spanish string literals in gameplay logic.
- **Missing Includes**: Fixed `TractorBeam.cpp` compilation errors by restoring `Config.hpp`.

---
## [Phase: Architecture & Chemistry Hardening] - 2025-12-26

### Added
- **JSON Chemistry Database**: Migrated hardcoded elements to `data/elements.json` for extensibility without recompilation.
- **JsonLoader.hpp**: New loader with comprehensive validation (Z-variance, color, required fields).
- **backgroundColor Field**: New element property for UI card styling.
- **DockingSystem.hpp**: Extracted auto-docking logic from Player for single-responsibility.
- **UndoManager.hpp**: Extracted hierarchical undo logic with attachment history tracking.
- **Mandatory Z-Axis Validation**: Game fails to start if elements lack Z-variance in bondingSlots.
- **MAX_BOND_RENDER_DIST**: Config constant to prevent visual glitches from overly stretched bonds.
- **test_molecular_geometry.cpp**: Automated tests for VSEPR angles and Z-variance.

### Changed
- **VSEPR Geometry Fixes**: Corrected bonding slots for O (104.5°), N (107°), P (93.5°), S (92°).
- **Player.cpp Refactoring**: Reduced from 164 → 120 lines by delegating to DockingSystem and UndoManager.
- **updateSpontaneousBonding()**: Optimized from O(N²) → O(N*k) using SpatialGrid.
- **Bonding Throttling**: Now runs at 10 Hz (every 6 frames) instead of every frame.
- **PhysicsEngine.cpp**: Z-axis now included in Hooke's Law spring calculations.

### Fixed
- **Atom Visual Overlap**: Phosphorus and other elements no longer overlap in 2.5D view.
- **Flickering Bonds**: Lowered render distance threshold to prevent valid bonds from disappearing.
- **Long Bond Lines**: Added max distance filter to hide stretched bonds that should have broken.
- **Game Freeze**: Reverted O(N²) Union-Find BFS implementation that caused performance regression.

---

## [Phase: Tractor Beam Refinement & Undo System] - 2025-12-26

### Added
- **Valencia Shield (Escudo de Valencia)**: New `isShielded` field in `StateComponent` prevents captured atoms from attracting "garbage" during transport.
- **Sticky Capture System**: Tractor beam now locks onto a single atom per click and won't auto-target others until released.
- **`becameActive()` API**: New method in `TractorBeam` detects the exact frame of capture for one-time operations.
- **Mouse-Based Dragging**: Atoms now follow the cursor position instead of being pulled directly toward the player.
- **Theme Constants**: Added `THEME_INFO` and `THEME_DANGER` colors to `Config.hpp` for notification system.

### Changed
- **Hierarchical Undo System**: Right-click now follows priority order:
  1. Self-release if player is captured by NPC molecule
  2. Chronological undo of manually attached atoms
  3. Prunable leaf search for natural molecule growth
- **Full Isolation Capture**: `breakAllBonds()` now executes only once on capture frame, preventing physics instability.
- **Global Shield Check**: Spontaneous bonding now checks root molecule's shield status, not individual atoms.
- **Tractor Physics Tuning**: 
  - `TRACTOR_FORCE`: 2.0 → 5.0
  - `TRACTOR_MAX_SPEED`: 250 → 500
  - `TRACTOR_PICKUP_RANGE`: 50 → 70
  - `BOND_DOCKING_SPEED`: 0.08 → 0.04 (smoother)
- **Player.cpp Reconstruction**: Complete rewrite to fix bracket imbalance and improve code clarity.

### Fixed
- **Vacuum Effect**: Fixed issue where tractor beam would suck up multiple atoms when moving cursor quickly.
- **Dead Valencia Bug**: Fixed bug where player couldn't bond captured atoms due to shield blocking self.
- **Garbage Clusters**: Atoms being towed no longer attract random neighbors during transport.
- **Compilation Errors**: Resolved missing `THEME_INFO` constant and structural bracket issues in `Player.cpp`.

---

## [Phase: Natural Chemistry Engine] - 2025-12-25

### Added
- **Coulomb Force Engine**: Real electromagnetic forces between atoms based on partial charges. Atoms attract/repel at distance before bonding.
- **Electronegativity System**: Full Pauling scale values for CHNOPS elements. Bonds now calculate polarity automatically.
- **Elastic Bonds (Hooke's Law)**: Molecules are no longer rigid; bonds now vibrate and flex like real chemical structures.
- **Stress-Based Bond Breaking**: NPCs' bonds auto-break when stretched beyond `BOND_BREAK_STRESS` threshold (35 units).
- **Player Immunity**: Player molecule (ID 0) is immune to stress rupture for fluid gameplay.
- **Config Constants**: New physics parameters (`COULOMB_CONSTANT`, `BOND_SPRING_K`, `BOND_BREAK_STRESS`, `BOND_IDEAL_DIST`, `EM_REACH`).

### Changed
- **PhysicsEngine.step()**: Now receives `atoms` and `states` vectors for chemistry-aware force calculations.
- **updateHierarchy**: Removed rigid position snapping; elastic forces in PhysicsEngine now handle molecular geometry.
- **Splice Bonding**: Improved validation - only atoms with `maxBonds >= 2` can act as bridges (H can never be a bridge).

### Fixed
- **Player Movement**: Player's molecule no longer breaks apart when moving at high speeds.
- **Compile Errors**: Added missing `ChemistryDatabase.hpp` include to PhysicsEngine.

---

## [Phase: Molecular Topology & Smart Chemistry] - 2025-12-25

### Added
- **Splice Bonding (Inserción por Empalme)**: Nuevo sistema que permite insertar átomos en medio de enlaces saturados (ej: de $H-H$ a $H-O-H$ de forma automática).
- **Total Valency Calculation**: El sistema ahora considera tanto enlaces entrantes como salientes, garantizando el cumplimiento estricto de las reglas de valencia (el Hidrógeno es ahora un terminal real).
- **Smart Molecule Scanner**: Al intentar unir un átomo, el sistema escanea automáticamente toda la molécula buscando slots libres mediante jerarquía dinámica (`findMoleculeRoot`).
- **Tractor Beam Soft-Capture**: Implementación de frenado progresivo con amortiguación dinámica basada en la distancia para evitar colisiones violentas.

### Changed
- **Tractor Physics**: Refinado de `TRACTOR_MAX_SPEED` (400 -> 250) y `STEER_FACTOR` para una respuesta más elástica y orgánica.
- **Auto-Acomodamiento**: La lógica de `tryBond` prioriza ahora el sitio de unión químicamente correcto sobre el punto de contacto físico si el modo es forzado por el jugador.

### Fixed
- **Saturación Molecular Falsa**: Corregido el error donde moléculas aparecían "Saturadas" prematuramente debido a IDs de molécula desincronizados.
- **H-H-O Invalid Topology**: El sistema ya no permite la formación de cadenas de hidrógeno inválidas, forzando la estructura $H-O-H$.

---

## [Phase Audit: Auditoría Técnica y Refactorización] - 2025-12-25

### Added
- **MathUtils.hpp**: Nueva cabecera de utilidades unificadas para generación de jitter y búsqueda de raíces moleculares (`findMoleculeRoot`).
- **Config.hpp Centralizado**: Estandarización de todas las constantes del motor a `inline constexpr`, eliminando "números mágicos" de los sistemas.
- **HUD Modular**: Extracción de la lógica de interfaz de `main.cpp` a un sistema dedicado `src/ui/HUD`.

### Changed
- **Refactorización de Sistemas**: Adaptación de `Player`, `BondingSystem`, `InputHandler` y `CameraSystem` para usar la nueva configuración centralizada.
- **Limpieza de Código**: Eliminación de logs redundantes y optimización de las constantes de interpolación de la cámara y el renderizado 2.5D.
- **Sistema de Construcción**: Actualización de `build.ps1` para incluir los nuevos módulos.

### Removed
- Archivos residuales de logs de errores previos y archivos temporales `.txt`.


## [Phase 12: Smooth Docking & Notifications] - 2025-12-25

### Added
- **Smooth Docking Animation**: New `dockingProgress` field in `StateComponent` enables lerp-based position interpolation when atoms bond.
- **NotificationManager**: New singleton UI system (`NotificationManager.hpp`) displays temporary messages with semi-transparent background.
- **Bond Failure Alerts**: "Enlace incompatible!" notification in red when attempting invalid chemical bonds.
- **File-Based Logging**: All `TraceLog` messages now persist to `session.log` via custom callback for debugging.

### Changed
- **updateHierarchy**: Now uses smooth interpolation (`lerp`) instead of instant teleportation for newly bonded atoms.
- **Tractor Beam Physics**: Restored jitter and smooth steering while attracting both free atoms and entire molecules.
- **Auto-Docking Logic**: Only attempts to bond free atoms; clustered atoms attract their whole molecule without auto-bonding.

### Fixed
- **Teleporting Atoms**: Atoms no longer snap instantly to their bonded position; they smoothly glide into place.
- **Tractor Beam Not Working**: Fixed issue where clicking on already-clustered atoms caused no physics response.

---

## [Phase 11: Bond Visualization Polish] - 2025-12-25

### Added
- **Edge-Based Bond Rendering**: Bonds now originate from atom surfaces, not centers, creating cleaner stick-and-ball visuals.
- **Atom-Blended Bond Colors**: Each bond uses the mixed color of its two connected atoms for organic appearance.
- **Shadow Lines**: Bonds have a black outline behind the color for depth matching the atom gradient style.

### Changed
- **Render Order**: Bonds now draw BEFORE atoms, ensuring they appear behind and don't overlap incorrectly.
- **Line Thickness**: Adjusted to 2.5px for balance between visibility and subtlety.
- **BOND_COMPRESSION**: Increased to 1.4 for clearer molecular structure visualization.

### Fixed
- **Bond Distance Scaling**: Fixed critical bug where bond distance used raw `vdWRadius` instead of visual radius (`vdWRadius * BASE_ATOM_RADIUS`).
- **Unused Variable Warning**: Removed `colorA` from `Renderer25D.cpp`.

---

## [Phase 10: Autonomous Molecular Evolution] - 2025-12-25

### Added
- **Spontaneous Bonding (`updateSpontaneousBonding`)**: NPC atoms now automatically form chemically accurate bonds using VSEPR rules.
- **BOND_AUTO_RANGE Config**: New constant (30.0f) controls the radius for autonomous bonding detection.
- **TractorBeam Diagnostics**: Added `TraceLog` messages for debugging atom detection and capture.

### Changed
- **Player Molecule Attraction**: Clicking on molecules now attracts the entire molecule (via root) rather than individual atoms.
- **Scale Rebalancing**: Restored `BASE_ATOM_RADIUS` to 6.0 for premium visual appearance.

### Fixed
- **Player Auto-Selection**: Tractor beam now correctly ignores the player (ID 0) to prevent self-targeting.

---

## [Phase 8: System Hardening & Optimization] - 2025-12-25

### Added
- **Loop Fusion (Physics)**: Consolidated motion integration, friction, and world bounds into a single optimized pass ($O(N)$), improving cache efficiency.
- **World Container (ECS)**: Encapsulated raw component vectors (`transforms`, `atoms`, `states`) into a unified `World` class for cleaner architecture.
- **Centralized Magic Numbers**: Moved 100% of hardcoded constants (`DRAG_COEFFICIENT`, `BOND_COMPRESSION`, etc.) to `Config.hpp`.
- **Dynamic Pre-pass Layout**: `UIWidgets` now support height-only calculation for perfect container scaling.

### Changed
- **Main Refactor**: Drastically simplified `main.cpp` by offloading initialization and state management to the `World` object.
- **Constexpr Standard**: Upgraded `Config` constants to `inline constexpr` for better compiler optimization.

### Fixed
- **UI Overflow**: Inspector panels now dynamically resize to prevent text clipping in long lore/origin strings.
- **Build Resilience**: Cleaned up Raylib API mismatches and compiler-specific errors like `std::clamp` scope.

---

## [Phase 7: Master Alchemy & UI Polish] - 2025-12-25

### Added
- **Full CHNOPS Set**: Integrated Phosphorus (P) and Sulfur (S) with custom lore, origins, and VSEPR geometries.
- **Premium Inspector**: Added "Glow Panel" effects and progress bars for molecular data visualization.
- **Dynamic Text Wrapping**: New `drawTextWrapped` system that updates the Y-coordinate for multi-line content flow.
- **VSEPR Templates**: Atoms now snap to specific geometric slots (Tetrahedral, Trigonal Pyramid, etc.) based on atomic type.

### Changed
- **Chemistry Database**: Expanded to include biogenetic lore and specific atomic origins (Nucleosynthesis).
- **Inspector Layout**: The "Analyze Molecule" button now flows naturally at the end of dynamic content.

---

## [Phase 6.5 & 6.6: Refinement & High-Performance] - 2025-12-25

### Added
- **Spatial Grid (Grid Hash)**: Implemented O(1) spatial partitioning system for neighbor lookups, collisions, and tools.
- **Fixed Timestep Accumulator**: Simulation logic now runs at a guaranteed 60Hz, decoupled from render frame rate for physics stability.
- **Renderer Decoupling**: Moved `Renderer25D` implementation to `.cpp`, significantly improving build times.
- **Centralized Movement**: `InputHandler` now manages WASD movement, abstracting player logic from engine-specific keys.

### Changed
- **O(1) Chemistry Database**: Replaced `std::map` with `std::vector` indexing for element access, eliminating O(log N) overhead in the hot loop.
- **Tractor Beam Optimization**: Now utilizes the `SpatialGrid` for localized O(1) searches instead of O(N) linear scans.
- **Config Standardization**: Centralized `PLAYER_SPEED` and `LABEL_FADE_SPEED` in `Config.hpp`.

### Fixed
- **Physics Jitter**: The fixed timestep eliminates "teleportation" or jitter issues at high or fluctuating framerates.
- **Input Conflicts**: Centralized input state prevents "click-through" issues between UI panels and the game world.

---

## [Phase 6: Visual & Interaction Overhaul] - 2025-12-25

### Added
- **UI Theme System**: Centralized `UI_THEME` in `Config.hpp` for consistent colors (Neo-Blue, Gold, Dark Glass).
- **UIWidgets Class**: New helper module for drawing standardized panels, headers, and separators.
- **Cinematic Camera**: Smooth zoom transitions ("Chill Zoom") with target-based interpolation.
- **Free Panning**: Right-click to pan the camera freely across the world.
- **Spacebar Reset**: Instant re-centering on the player with smooth zoom-to-2.0x transition.
- **Dynamic LOD Labels**: SSymbols (H, C, O, N) stay visible until 0.45x zoom, then smoothly transition to Molecule names.
- **Player Inspector**: Pressing Space now triggers an analysis of the player's own atom.
- **Molecule View Placeholder**: Double-tapping Spacebar opens a mock-up for molecule analysis.

### Changed
- **Nano-Scale UI**: All UI elements (HUD, Inspector) scaled down by ~40% for a non-intrusive aesthetic.
- **Text Crispness**: Replaced "faux-bold" with high-clarity single-pass rendering for small fonts.
- **Architecture**: Modularized `main.cpp` by extracting world initialization and HUD rendering.
- **Config Audit**: Eliminated 100% of magic numbers in UI and rendering modules, moving them to `Config.hpp`.

### Fixed
- **Window Resize**: Camera offset now updates dynamically every frame ensuring consistent centering.
- **UI Depth**: Improved depth-sorting and color contrast for atoms in the 2.5D renderer.

---

## [Phase 5: ECS & Rendering Base] 
- Implementation of the Core ECS (Entity Component System).
- 2.5D Rendering engine with depth perception.
- Interactive Physics engine foundation.
- Chemistry Database with basic Element properties.
