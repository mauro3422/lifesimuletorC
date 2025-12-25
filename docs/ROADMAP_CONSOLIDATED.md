# Roadmap Consolidado: LifeSimulator C++

Este documento hereda la visión del proyecto original en Python pero enfocado en la nueva arquitectura de C++.

## 🎯 Objetivos de la Migración
- [ ] Pasar de 5k a 50k - 100k partículas.
- [ ] Implementar **Rigid Body Clustering** (las moléculas estables no calculan física interna).
- [ ] Interfaz nativa ultra-rápida con ImGui.
- [ ] Portar toda la base de datos científica (140+ moléculas).

## 🚀 Fases de Desarrollo

### Fase 1: Motor Base (Actual)
- [ ] Setup de Raylib/SFML.
- [ ] Implementación de un ECS simple (Entities & Components).
- [ ] Renderizado por partículas (Point Sprites o Instancing).
- [ ] Colisiones espaciales (Grid Optimization).

### Fase 2: Biofísica Optimizada
- [ ] Enlaces tipo muelle (Springs).
- [ ] Lógica de "soldar" moléculas (Clustering).
- [ ] Cargas parciales (Electronegatividad).

### Fase 3: Gameplay & Biología
- [ ] Player "Tractor Beam".
- [ ] Sistema de ATP y Metabolismo.
- [ ] Quimidex educativa nativa.

### Fase 4: IA & Emergencia
- [ ] Replicación de ARN/DNA.
- [ ] Enemigos (Bacterias, Virus).

---
*Referencia original: `xd/ROADMAP.md`*
