# Changelog - LifeSimulator C++

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
