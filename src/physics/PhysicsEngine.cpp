#include "PhysicsEngine.hpp"
#include "../core/Config.hpp"
#include <cmath>
#include <algorithm>
#include "../chemistry/ChemistryDatabase.hpp"

PhysicsEngine::PhysicsEngine() : grid(Config::GRID_CELL_SIZE) {}

void PhysicsEngine::step(float dt, std::vector<TransformComponent>& transforms,
                        const std::vector<AtomComponent>& atoms,
                        const std::vector<StateComponent>& states) {
    // 0. ACTUALIZAR GRID (Necesaria para fuerzas de largo alcance EM)
    grid.update(transforms);

    // 1. APLICAR FUERZAS ELECTROMAGNÉTICAS (Coulomb O(N))
    for (int i = 0; i < (int)transforms.size(); i++) {
        float q1 = atoms[i].partialCharge;
        if (std::abs(q1) < 0.001f) continue; // Átomo neutro, no genera campo EM significativo

        // Buscar vecinos en la grilla para aplicar fuerzas
        std::vector<int> neighbors = grid.getNearby({transforms[i].x, transforms[i].y}, Config::EM_REACH);
        for (int j : neighbors) {
            if (i == j) continue;
            float q2 = atoms[j].partialCharge;
            if (std::abs(q2) < 0.001f) continue;

            float dx = transforms[j].x - transforms[i].x;
            float dy = transforms[j].y - transforms[i].y;
            float distSq = dx*dx + dy*dy + 0.1f;
            float dist = std::sqrt(distSq);

            if (dist > Config::EM_REACH) continue;

            // Ley de Coulomb: F = k * (q1 * q2) / r^2
            // Limitamos r para evitar fuerzas infinitas (Repulsión de Pauli/Soft-Core)
            float effectiveDist = std::max(dist, Config::MIN_COULOMB_DIST);
            float forceMag = (Config::COULOMB_CONSTANT * q1 * q2) / (effectiveDist * effectiveDist);
            
            // Vector unitario de la fuerza
            float fx = (dx / dist) * forceMag;
            float fy = (dy / dist) * forceMag;

            // Aplicar aceleración (F = ma, asumimos m=1 por ahora)
            transforms[i].vx -= fx * dt;
            transforms[i].vy -= fy * dt;
            transforms[j].vx += fx * dt;
            transforms[j].vy += fy * dt;
        }
    }

    // 2. ENLACES ELÁSTICOS Y ESTRÉS MOLECULAR (Geometría Dinámica)
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (!states[i].isClustered || states[i].parentEntityId == -1) continue;

        int parentId = states[i].parentEntityId;
        int slotIdx = states[i].parentSlotIndex;

        // Obtener datos del padre para calcular el slot ideal
        const Element& parentElem = ChemistryDatabase::getInstance().getElement(atoms[parentId].atomicNumber);
        if (slotIdx >= (int)parentElem.bondingSlots.size()) continue;

        Vector3 slotDir = parentElem.bondingSlots[slotIdx];
        // En un motor real rotaríamos slotDir por la rotación del padre
        // Aquí simplificamos asumiendo rotación por ahora (o añadir rotación al Transform)
        
        float targetX = transforms[parentId].x + slotDir.x * Config::BOND_IDEAL_DIST;
        float targetY = transforms[parentId].y + slotDir.y * Config::BOND_IDEAL_DIST;

        float dx = targetX - transforms[i].x;
        float dy = targetY - transforms[i].y;
        float dist = std::sqrt(dx*dx + dy*dy);

        // --- RUPTURA POR ESTRÉS ---
        // EXCEPCIÓN: La molécula del jugador (ID 0) es inmune a la ruptura para permitir movimiento libre
        bool isPlayerMolecule = (states[i].moleculeId == 0 || i == 0 || parentId == 0);
        
        if (!isPlayerMolecule && dist > Config::BOND_BREAK_STRESS) {
            const_cast<StateComponent&>(states[i]).isClustered = false;
            const_cast<StateComponent&>(states[i]).parentEntityId = -1;
            TraceLog(LOG_WARNING, "[PHYSICS] ENLACE ROTO por estres: Atomo %d se separo de %d", i, parentId);
            continue;
        }

        // --- LEY DE HOOKE (Fuerza de Restauración) ---
        float fx = dx * Config::BOND_SPRING_K;
        float fy = dy * Config::BOND_SPRING_K;

        // Aplicar a ambos (Acción y Reacción)
        transforms[i].vx += fx * dt;
        transforms[i].vy += fy * dt;
        transforms[parentId].vx -= fx * dt;
        transforms[parentId].vy -= fy * dt;
    }

    // 3. FUSIÓN DE LOOPS: Integración, Fricción y Límites en un solo paso
    for (TransformComponent& tr : transforms) {
        // 1. Integración
        tr.x += tr.vx * dt;
        tr.y += tr.vy * dt;
        tr.z += tr.vz * dt;

        // 2. Fricción Ambiental (Configurable)
        tr.vx *= Config::DRAG_COEFFICIENT;
        tr.vy *= Config::DRAG_COEFFICIENT;
        tr.vz *= Config::DRAG_COEFFICIENT;

        // 3. Límites del Mundo (Z)
        if (tr.z < Config::WORLD_DEPTH_MIN) {
            tr.z = Config::WORLD_DEPTH_MIN;
            tr.vz *= Config::WORLD_BOUNCE;
        } else if (tr.z > Config::WORLD_DEPTH_MAX) {
            tr.z = Config::WORLD_DEPTH_MAX;
            tr.vz *= Config::WORLD_BOUNCE;
        }
    }

    // Actualizar Grid Espacial
    grid.update(transforms);
}
