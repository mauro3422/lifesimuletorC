# 🧪 LifeSimulator C++

**High-Performance Molecular Evolution Engine**

Este proyecto es la evolución de `LifeSimulator` de Python a C++. El objetivo es alcanzar una simulación masiva de física química acelerada por hardware, con una estética "Nano-HD" minimalista y funcional.

## 🕹️ Controles Principales

- **WASD**: Navegar por el entorno.
- **Mouse Wheel**: Control de Zoom (Transiciones suaves).
- **Click Izquierdo**: Activar Rayo Tractor (Captura quirúrgica de átomos).
- **Click Derecho**: **Desacoplar átomo** (Undo jerárquico) / **Paneo Libre** (Hold).
- **Espacio**: Centrar cámara en el Avatar + Abrir Inspector de Elemento.
- **Doble Espacio**: Abrir Vista de Molécula.
- **F11**: Pantalla Completa.

## ✨ Características Principales

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
- `src/core/`: Configuración global y constantes.
- `src/ui/`: `Inspector`, `LabelSystem`, `UIWidgets`, `NotificationManager`.
- `src/rendering/`: Cámara cinemática y Render 2.5D.
- `src/chemistry/`: Base de datos de elementos y propiedades.
- `src/physics/`: `BondingSystem`, `PhysicsEngine`, `SpatialGrid`.
- `src/gameplay/`: `Player`, `TractorBeam`.

## 🛠️ Roadmap Actualizado
- [x] **Fase 5**: ECS & Render Base
- [x] **Fase 6**: Rediseño Visual & Interacción Nano-HD
- [x] **Fase 7**: Master Alchemy (CHNOPS, VSEPR, Dynamic UI)
- [x] **Fase 8**: System Hardening & Deep Optimization
- [x] **Fase 10**: Autonomous Molecular Evolution
- [x] **Fase 11**: Bond Visualization Polish
- [x] **Fase 12**: Smooth Docking & Notifications
- [x] **Audit**: Molecular Topology & Smart Chemistry
- [x] **Natural Chemistry**: Coulomb, Electronegativity, Elastic Bonds
- [x] **Tractor Beam Refinement**: Valencia Shield, Sticky Capture, Hierarchical Undo
- [ ] **Fase 13**: Bio-Génesis (ATP, Metabolismo Inicial)

---
*Basado en el diseño original de LifeSimulator Python.*
