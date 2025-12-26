#include "PhysicsEngine.hpp"
#include "../core/Config.hpp"
#include <cmath>
#include <algorithm>
#include "../chemistry/ChemistryDatabase.hpp"
#include "../core/MathUtils.hpp"

PhysicsEngine::PhysicsEngine() : grid(Config::GRID_CELL_SIZE) {}

void PhysicsEngine::step(float dt, std::vector<TransformComponent>& transforms,
                        const std::vector<AtomComponent>& atoms,
                        std::vector<StateComponent>& states) {
    // 0. UPDATE ENVIRONMENT (The grid will be updated at the end of the step)
    environment.update(transforms, states, dt); 

    // 1. APPLY ELECTROMAGNETIC FORCES (Coulomb O(N))
    for (int i = 0; i < (int)transforms.size(); i++) {
        float q1 = atoms[i].partialCharge;
        if (std::abs(q1) < Config::CHARGE_THRESHOLD) continue; // Neutral atom, no significant EM field

        // Search for neighbors in the grid to apply forces
        std::vector<int> neighbors = grid.getNearby({transforms[i].x, transforms[i].y}, Config::EM_REACH);
        for (int j : neighbors) {
            if (i == j) continue;
            float q2 = atoms[j].partialCharge;
            if (std::abs(q2) < Config::CHARGE_THRESHOLD) continue;

            float distSq = MathUtils::distSq(transforms[i].x, transforms[i].y, transforms[j].x, transforms[j].y);
            // Pre-check squared distance to avoid sqrt if not needed (optimization)
            if (distSq > Config::EM_REACH * Config::EM_REACH) continue;
            
            float dist = std::sqrt(distSq + (Config::PHYSICS_EPSILON * Config::PHYSICS_EPSILON));

            if (dist > Config::EM_REACH) continue;

            // Coulomb's Law: F = k * (q1 * q2) / r^2
            // Clamp r to avoid infinite forces (Pauli Repulsion/Soft-Core)
            float effectiveDist = std::max(dist, Config::MIN_COULOMB_DIST);
            float forceMag = (Config::COULOMB_CONSTANT * q1 * q2) / (effectiveDist * effectiveDist);
            
            float dx = transforms[j].x - transforms[i].x;
            float dy = transforms[j].y - transforms[i].y;
            // Force unit vector
            float fx = (dx / dist) * forceMag;
            float fy = (dy / dist) * forceMag;

            // Apply acceleration (a = F / m)
            float m1 = ChemistryDatabase::getInstance().getElement(atoms[i].atomicNumber).atomicMass;
            float m2 = ChemistryDatabase::getInstance().getElement(atoms[j].atomicNumber).atomicMass;
            
            // Handle potentially zero mass from JSON (fallback to 1.0)
            if (m1 < 0.01f) m1 = 1.0f;
            if (m2 < 0.01f) m2 = 1.0f;

            transforms[i].vx -= (fx / m1) * dt;
            transforms[i].vy -= (fy / m1) * dt;
            transforms[j].vx += (fx / m2) * dt;
            transforms[j].vy += (fy / m2) * dt;
        }
    }

    // 2. ELASTIC BONDS AND MOLECULAR STRESS (Dynamic Geometry)
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (!states[i].isClustered || states[i].parentEntityId == -1) continue;

        int parentId = states[i].parentEntityId;
        int slotIdx = states[i].parentSlotIndex;

        // Get parent data to calculate ideal slot position
        const Element& parentElem = ChemistryDatabase::getInstance().getElement(atoms[parentId].atomicNumber);
        if (slotIdx >= (int)parentElem.bondingSlots.size()) continue;

        Vector3 slotDir = parentElem.bondingSlots[slotIdx];
        
        // Calculate target position in 3D (including Z for correct 2.5D)
        float targetX = transforms[parentId].x + slotDir.x * Config::BOND_IDEAL_DIST;
        float targetY = transforms[parentId].y + slotDir.y * Config::BOND_IDEAL_DIST;
        float targetZ = transforms[parentId].z + slotDir.z * Config::BOND_IDEAL_DIST;

        float dx = targetX - transforms[i].x;
        float dy = targetY - transforms[i].y;
        float dz = targetZ - transforms[i].z;
        float dist = MathUtils::length(dx, dy, dz);

        // --- STRESS BREAKUP ---
        bool isPlayerMolecule = (states[i].moleculeId == 0 || i == 0 || parentId == 0);
        
        if (!isPlayerMolecule && dist > Config::BOND_BREAK_STRESS) {
            states[i].isClustered = false;
            states[i].parentEntityId = -1;
            TraceLog(LOG_WARNING, "[PHYSICS] BOND BROKEN by stress: Atom %d separated from %d", i, parentId);
            continue;
        }

        // --- HOOKE'S LAW (Restoration Force) with Z ---
        float fx = dx * Config::BOND_SPRING_K;
        float fy = dy * Config::BOND_SPRING_K;
        float fz = dz * Config::BOND_SPRING_K;

        // Apply acceleration based on mass
        float m1 = ChemistryDatabase::getInstance().getElement(atoms[i].atomicNumber).atomicMass;
        float mP = parentElem.atomicMass;
        if (m1 < 0.01f) m1 = 1.0f;
        if (mP < 0.01f) mP = 1.0f;

        // Apply to both (Action and Reaction) including Z
        transforms[i].vx += (fx / m1) * dt;
        transforms[i].vy += (fy / m1) * dt;
        transforms[i].vz += (fz / m1) * dt;
        
        transforms[parentId].vx -= (fx / mP) * dt;
        transforms[parentId].vy -= (fy / mP) * dt;
        transforms[parentId].vz -= (fz / mP) * dt;
    }

    // 3. LOOP FUSION: Integration, Friction, and Boundaries in one step
    for (TransformComponent& tr : transforms) {
        // 1. Integration
        tr.x += tr.vx * dt;
        tr.y += tr.vy * dt;
        tr.z += tr.vz * dt;

        // 2. Ambient Friction (Configurable)
        tr.vx *= Config::DRAG_COEFFICIENT;
        tr.vy *= Config::DRAG_COEFFICIENT;
        tr.vz *= Config::DRAG_COEFFICIENT;

        // 3. World Boundaries (Z)
        if (tr.z < Config::WORLD_DEPTH_MIN) {
            tr.z = Config::WORLD_DEPTH_MIN;
            tr.vz *= Config::WORLD_BOUNCE;
        } else if (tr.z > Config::WORLD_DEPTH_MAX) {
            tr.z = Config::WORLD_DEPTH_MAX;
            tr.vz *= Config::WORLD_BOUNCE;
        }
    }

    // Update Spatial Grid
    grid.update(transforms);
}
