# 🧪 LifeSimulator C++

**High-Performance Molecular Evolution Engine**

Este proyecto es la evolución de `LifeSimulator` de Python a C++. El objetivo es alcanzar una simulación masiva de física química acelerada por hardware, con una estética "Nano-HD" minimalista y funcional.

![Emergent Molecular Chains](screenshot_molecules.png)
*Formación emergente de cadenas moleculares usando física VSEPR y fuerzas de Coulomb*

## 🕹️ Controles Principales

- **WASD**: Navegar por el entorno.
- **Mouse Wheel**: Control de Zoom (Transiciones suaves).
- **Click Izquierdo**: Activar Rayo Tractor (Captura quirúrgica de átomos).
- **Click Derecho**: **Desacoplar átomo** (Undo jerárquico) / **Paneo Libre** (Hold).
- **Espacio**: Centrar cámara en el Avatar + Abrir Inspector de Elemento.
- **Doble Espacio**: Abrir Vista de Molécula.
- **F11**: Pantalla Completa.

## ✨ Características Principales

- **Multilingual Support**: Full English/Spanish localization with runtime toggle (**F1**).
- **Mass-Based Inertia**: Simulation uses $F=ma$; heavy elements feel heavier.
- **High-Performance Bonding**: O(1) slot detection algorithm for massive molecules.
- **Física Química Real**: Motor basado en reglas científicas (Electronegatividad, Coulomb, Hooke).
- **Química Inteligente**: Los átomos respetan valencias totales y permiten el **auto-acomodamiento** (Splice Bonding).
- **Enlaces Elásticos**: Las moléculas vibran y pueden romperse bajo estrés (excepto la del jugador).
- **Soft-Capture Tractor**: Captura de precisión con frenado progresivo y amortiguación elástica.
- **Smooth Docking**: Animación suave cuando los átomos se acoplan a moléculas.
- **Visualización Ball-and-Stick**: Enlaces con colores mezclados y sombras de profundidad.
- **Logging Persistente**: Todos los eventos se guardan en `session.log`.

## 🚀 Visión Técnica
- **Motor**: C++17 con Raylib 5.0 (High-DPI enabled).
- **Arquitectura**: Clean ECS (Entity Component System).
- **Estética**: "LORE-CORE" / Nano-HD (Escalado compacto, tipografía ultra-nítida).
- **UI**: Sistema modular de `UIWidgets` para consistencia visual.

## 📂 Estructura

```
LifeSimulatorCPP/
├── src/
│   ├── core/           # Config, MathUtils, Localization, ErrorHandling
│   ├── ecs/            # World, Components
│   ├── physics/        # PhysicsEngine, BondingSystem, SpatialGrid, RingChemistry
│   ├── chemistry/      # ChemistryDatabase, StructureRegistry
│   ├── gameplay/       # Player, TractorBeam, MissionManager
│   ├── rendering/      # Renderer25D, CameraSystem
│   ├── ui/             # Inspector, HUD, Quimidex, UIWidgets
│   ├── input/          # InputHandler
│   └── world/          # Zones (ClayZone)
├── data/               # JSON: elements, molecules, structures, localization
├── tests/              # Integration tests (doctest)
├── docs/               # Documentation
└── external/           # Raylib 5.0
```

📖 **Documentación adicional:**
- [BUILDING.md](docs/BUILDING.md) - Instrucciones de compilación
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - Diagrama del sistema

## 🛠️ Roadmap Actualizado
- [x] **Fase 5**: ECS & Render Base
- [x] **Fase 6**: Rediseño Visual & Interacción Nano-HD
- [x] **Fase 7**: Master Alchemy (CHNOPS, VSEPR, Dynamic UI)
- [x] **Fase 8**: System Hardening & Deep Optimization
- [x] **Fase 10**: Autonomous Molecular Evolution
- [x] **Fase 11**: Bond Visualization Polish
- [x] **Fase 12**: Smooth Docking & Notifications
- [x] **Natural Chemistry**: Coulomb, Electronegativity, Elastic Bonds
- [x] **Tractor Refinement**: Valencia Shield, Sticky Capture, Hierarchical Undo
- [x] **Architecture Hardening**: JSON DB, Player Refactor, VSEPR Validation
- [x] **Fase 17**: Deep Audit (Mass Physics, O(1) Bonding, Bilingual UI)
- [x] **Fase 30**: Architectural Standardization (De-God-Class, ErrorHandler)
- [x] **Fase 31**: Test Coverage & Code Quality (43 tests, Comment Standardization)
- [ ] **Fase 18**: Chemical Expansion (Transition Metals, Complex Organics)
- [ ] **Fase 19**: Exotic States (Plasma, Supercritical Fluids)
- [ ] **Fase 20**: Bio-Genesis (ATP, Metabolismo Inicial)

---
*Basado en el diseño original de LifeSimulator Python.*
