#include "PhysicsEngine.hpp"
#include "BondingSystem.hpp"
#include "StructuralPhysics.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../chemistry/StructureDefinition.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include <cmath>
#include <algorithm>
#include <map>
#include "../core/ErrorHandling.hpp"

PhysicsEngine::PhysicsEngine() : grid(Config::GRID_CELL_SIZE) {}

void PhysicsEngine::step(float dt, std::vector<TransformComponent>& transforms,
                        std::vector<AtomComponent>& atoms,
                        std::vector<StateComponent>& states,
                        const ChemistryDatabase& db,
                        int tractedEntityId) {
    if (atoms.size() != transforms.size() || atoms.size() != states.size()) {
        ErrorHandler::handle(ErrorSeverity::FATAL, "Component size mismatch: atoms=%zu, transforms=%zu, states=%zu", 
                             atoms.size(), transforms.size(), states.size());
        return;
    }
    // 0. UPDATE ENVIRONMENT (The grid will be updated at the end of the step)
    static int diagCounter = 0;
    environment.update(transforms, states, dt); 

    // 0.5 PRECALCULATE ROOTS (Phase 28 Optimization)
    // Avoids redundant findMoleculeRoot calls in hot loops
    std::vector<int> rootCache(transforms.size());
    for (int i = 0; i < (int)transforms.size(); i++) {
        rootCache[i] = MathUtils::findMoleculeRoot(i, states);
    }

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
            
            // USE SAFE NORMALIZE (Fix #1)
            Vector3 diff = {dx, dy, 0.0f};
            Vector3 dir = MathUtils::safeNormalize(diff);
            
            float fx = dir.x * forceMag;
            float fy = dir.y * forceMag;

            // Apply acceleration (a = F / m)
            float m1 = db.getElement(atoms[i].atomicNumber).atomicMass;
            float m2 = db.getElement(atoms[j].atomicNumber).atomicMass;
            
            if (m1 < 0.01f) m1 = 1.0f;
            if (m2 < 0.01f) m2 = 1.0f;

            // BUGFIX: Player Force Clamping
            if (i == 0) { 
                float maxF = 150.0f; 
                fx = std::clamp(fx, -maxF, maxF);
                fy = std::clamp(fy, -maxF, maxF);
            }

            transforms[i].vx -= (fx / m1) * dt;
            transforms[i].vy -= (fy / m1) * dt;
            transforms[j].vx += (fx / m2) * dt;
            transforms[j].vy += (fy / m2) * dt;
            
            // BUGFIX: Clamp Coulomb Speed (Fix #6)
            constexpr float MAX_COULOMB_SPEED = 600.0f;
            MathUtils::ClampMagnitude(transforms[i].vx, transforms[i].vy, MAX_COULOMB_SPEED);
            MathUtils::ClampMagnitude(transforms[j].vx, transforms[j].vy, MAX_COULOMB_SPEED);
        }
    }

    // 2. ELASTIC BONDS AND MOLECULAR STRESS (Dynamic Geometry)
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (!states[i].isClustered || states[i].parentEntityId == -1) continue;

        int parentId = states[i].parentEntityId;
        int slotIdx = states[i].parentSlotIndex;

        // Get parent data to calculate ideal slot position
        const Element& parentElem = db.getElement(atoms[parentId].atomicNumber);
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

        // --- RING vs NORMAL BOND PHYSICS ---
        float fx, fy, fz;
        
        if (states[i].isInRing && states[parentId].isInRing) {
            // RING BONDS: Use DISTANCE-ONLY forces (no VSEPR direction)
            // This lets the angular forces control geometry instead
            float actualDx = transforms[parentId].x - transforms[i].x;
            float actualDy = transforms[parentId].y - transforms[i].y;
            float actualDz = transforms[parentId].z - transforms[i].z;
            float actualDist = MathUtils::length(actualDx, actualDy, actualDz);
            
            if (actualDist > 0.1f) {
                // Calculate spring force based on distance deviation only
                // Lower spring since square is positioned correctly at formation
                float strain = actualDist - Config::BOND_IDEAL_DIST;
                float ringSpringK = Config::BOND_SPRING_K * Config::Physics::RING_SPRING_MULTIPLIER; 
                float forceMag = strain * ringSpringK;
                
                // Normalize direction
                float nx = actualDx / actualDist;
                float ny = actualDy / actualDist;
                float nz = actualDz / actualDist;
                
                fx = nx * forceMag;
                fy = ny * forceMag;
                fz = nz * forceMag;
                
                // BUG FIX: Clamp spring forces to prevent explosions
                fx = std::clamp(fx, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
                fy = std::clamp(fy, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
                fz = std::clamp(fz, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
            } else {
                fx = fy = fz = 0;
            }
        } else {
            // NORMAL BONDS: Use VSEPR slot direction (original behavior)
            // --- HOOKE'S LAW (Restoration Force) with Z ---
            fx = dx * Config::BOND_SPRING_K;
            fy = dy * Config::BOND_SPRING_K;
            fz = dz * Config::BOND_SPRING_K;
            
            // BUG FIX: Clamp spring forces to prevent explosions
            fx = std::clamp(fx, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
            fy = std::clamp(fy, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
            fz = std::clamp(fz, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
        }

        // Apply acceleration based on mass
        float m1 = db.getElement(atoms[i].atomicNumber).atomicMass;
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
        
        // STRESS DIAGNOSTICS (Refined)
        // Log ONLY for atoms connected to Player (Entity 0) or high stress to prevent spam
        if (diagCounter > 120) { // Check every 2 seconds (uses counter incremented at end of frame)
             if (states[parentId].moleculeId == 0) { // Only Player's structure
                 float strain = (dist - Config::BOND_IDEAL_DIST);
                 // Only log if significant deviation (> 20%) to avoid noise
                 if (abs(strain) > 5.0f) {
                     TraceLog(LOG_INFO, "[STRESS] Bond %d->%d (Slot %d) | Dist: %.1f / %.1f | Strain: %.1f", 
                              parentId, i, slotIdx, dist, Config::BOND_IDEAL_DIST, strain);
                 }
             }
        }
    }

    // --- PHASE 18: CYCLE BONDS (Non-Hierarchical Springs) ---
    // Uses SAME gentle physics as other ring bonds for uniform behavior
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (states[i].cycleBondId == -1) continue;

        int partnerId = states[i].cycleBondId;
        // Avoid double processing (process only if i < partnerId)
        if (i > partnerId) continue; 

        float dx = transforms[partnerId].x - transforms[i].x;
        float dy = transforms[partnerId].y - transforms[i].y;
        float dz = transforms[partnerId].z - transforms[i].z;
        float dist = MathUtils::length(dx, dy, dz);

        if (dist < 0.1f) continue;

        // Use STRONGER spring for structural stability
        float strain = dist - Config::BOND_IDEAL_DIST;
        float ringSpringK = Config::BOND_SPRING_K * Config::Physics::RING_SPRING_MULTIPLIER; 
        float forceMag = strain * ringSpringK;
        
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;
        
        float fx = nx * forceMag;
        float fy = ny * forceMag;
        float fz = nz * forceMag;
        
        // BUG FIX: Clamp spring forces to prevent explosions
        fx = std::clamp(fx, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
        fy = std::clamp(fy, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
        fz = std::clamp(fz, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);

        float m1 = db.getElement(atoms[i].atomicNumber).atomicMass;
        float m2 = ChemistryDatabase::getInstance().getElement(atoms[partnerId].atomicNumber).atomicMass;
        if (m1 < 0.01f) m1 = 1.0f;
        if (m2 < 0.01f) m2 = 1.0f;

        transforms[i].vx += (fx / m1) * dt;
        transforms[i].vy += (fy / m1) * dt;
        transforms[i].vz += (fz / m1) * dt;

        transforms[partnerId].vx -= (fx / m2) * dt;
        transforms[partnerId].vy -= (fy / m2) * dt;
        transforms[partnerId].vz -= (fz / m2) * dt;
    }

    // --- PHASE 23: STRUCTURAL DYNAMICS (Rings & Rigid Groups) ---
    // Offloaded to specialized module for code hygiene
    StructuralPhysics::applyRingDynamics(dt, transforms, atoms, states, rootCache);

    // --- PHASE 32: FOLDING & AFFINITY (Catalytic Synthesis) ---
    StructuralPhysics::applyFoldingAndAffinity(dt, transforms, atoms, states, environment, rootCache);

    // --- PHASE 27+: SPONTANEOUS BONDING (Autonomous Evolution) ---
    // Pass rootCache to optimize molecule detection
    BondingSystem::updateSpontaneousBonding(states, atoms, transforms, grid, rootCache, &environment, tractedEntityId);

    // 3. LOOP FUSION: Integration, Friction, and Boundaries in one step
    for (TransformComponent& tr : transforms) {
        // 1. Integration (With Thermodynamic Jitter)
        float jitterX = MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
        float jitterY = MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
        float jitterZ = MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER * 0.2f;

        tr.vx += jitterX * dt;
        tr.vy += jitterY * dt;
        tr.vz += jitterZ * dt;

        tr.x += tr.vx * dt;
        tr.y += tr.vy * dt;
        tr.z += tr.vz * dt;

        // Phase 29: Hard Snap Z=0 for established rings to prevent drift
        if (states[&tr - &transforms[0]].isInRing && states[&tr - &transforms[0]].isLocked()) {
            tr.z = 0.0f;
            tr.vz = 0.0f;
        }

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
    diagCounter++;
    if (diagCounter > 120) diagCounter = 0;
    grid.update(transforms);

    // Phase 29: Reset frame-local flags
    for (auto& s : states) s.justBonded = false;
}
