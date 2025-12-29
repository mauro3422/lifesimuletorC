# 🗺️ Roadmap: LifeSimulator C++

Este documento refleja el estado actual del proyecto y las metas futuras.

---

## ✅ Fases Completadas

### Motor Base (Fases 5-8)
- [x] Setup de Raylib 5.0 con High-DPI
- [x] Implementación ECS (Entity Component System)
- [x] Renderizado 2.5D con profundidad
- [x] Grid espacial O(1) para colisiones
- [x] Timestep fijo 60Hz para física estable

### Química y Física (Fases 10-17)
- [x] Motor de Coulomb (Fuerzas electromagnéticas)
- [x] Electronegatividad y cargas parciales
- [x] Enlaces elásticos (Hooke's Law)
- [x] Geometría VSEPR para slots de bonding
- [x] Ruptura de enlaces por estrés
- [x] Base de datos JSON-driven (`elements.json`)

### Gameplay (Fases 12-17)
- [x] Player con Tractor Beam de precisión
- [x] Sistema de Undo jerárquico
- [x] Smooth Docking Animation
- [x] Sistema de notificaciones
- [x] Inspector de átomos y moléculas
- [x] Quimidex educativa nativa

### Estructuras y Anillos (Fases 18-27)
- [x] Cycle Bonds (enlaces no-jerárquicos)
- [x] Formación de anillos de 4 átomos (C4)
- [x] Hard Snap geométrico para estabilidad
- [x] Zone System (Clay Island como catalizador)
- [x] Thermodynamic Jitter (movimiento browniano)

### Arquitectura (Fases 28-37)
- [x] De-God-Classing del BondingSystem
- [x] Módulos especializados (BondingCore, RingChemistry, AutonomousBonding)
- [x] ErrorHandler unificado
- [x] 43+ tests unitarios
- [x] Localización bilingüe (ES/EN) con toggle F1
- [x] O(1) slot detection con bitmask

---

## 🚧 En Progreso

### Fase 38: Estabilización
- [ ] Resolver conflicto Raylib+Doctest en tests
- [ ] Mejorar cobertura de tests (60% → 80%)

---

## 🔮 Fases Futuras

### Fase 18+: Expansión Química
- [ ] Metales de transición (Fe, Mg, Zn, Cu)
- [ ] Compuestos orgánicos complejos
- [ ] 140+ moléculas del catálogo biológico

### Fase 19: Estados Exóticos
- [ ] Plasma y fluidos supercríticos
- [ ] Condiciones extremas (temperatura, presión)

### Fase 20: Bio-Génesis
- [ ] Sistema de ATP y metabolismo
- [ ] Aminoácidos y nucleótidos
- [ ] Cadenas de ARN funcionales

### Fase 20.5: Construcción de Membranas 🆕
- [ ] **Fusión de Hexágonos**: Pegar C6 por lado compartido
- [ ] **Crecimiento de Placas**: Encadenar hexágonos en malla
- [ ] **Curvatura Automática**: La placa se curva al crecer
- [ ] **Cierre de Membrana**: Detectar cuando forma "burbuja"
- [ ] **Detección Interior/Exterior**: Partículas adentro vs afuera
- [ ] **Impermeabilidad**: Colisiones contra muro de hexágonos

### Fase 21+: Automatización
- [ ] Ribosomas (lectores de ARN → proteínas)
- [ ] Polimerasas (copiadoras de ADN)
- [ ] ATP Sintasa (generación de energía)

### Fase 22+: Competencia IA
- [ ] Bacterias rivales
- [ ] Virus y fagos
- [ ] Modos: Survival, Race, Arena, Sandbox

---

## 🎯 Metas de Rendimiento

| Métrica | Actual | Objetivo |
|---------|--------|----------|
| Partículas | 2,500 | 50,000+ |
| FPS | 60 | 60 estable |
| Tiempo de carga | <2s | <1s |

---

*Actualizado: 2025-12-28*
