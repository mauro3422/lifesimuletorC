#include "PhysicsEngine.hpp"
#include "BondingSystem.hpp"
#include "StructuralPhysics.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../chemistry/StructureDefinition.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "RingChemistry.hpp"
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include "../core/ErrorHandling.hpp"

PhysicsEngine::PhysicsEngine() : grid(Config::GRID_CELL_SIZE) {}

// ============================================================================
// HELPER: Validate Ring Integrity
// Cleans up orphaned ring markers for atoms no longer in valid rings
// ============================================================================
void PhysicsEngine::validateRingIntegrity(std::vector<StateComponent>& states) {
    // First pass: identify active rings (supported by a mutual cycle bond)
    std::set<int> activeRingIds;
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isInRing && states[i].cycleBondId != -1) {
            int partner = states[i].cycleBondId;
            if (partner >= 0 && partner < (int)states.size() && states[partner].cycleBondId == i) {
                if (states[i].ringInstanceId != -1) {
                    activeRingIds.insert(states[i].ringInstanceId);
                }
            }
        }
    }
    
    // Second pass: invalidate any atom whose ringId is not in the active set
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isInRing) {
            int ringId = states[i].ringInstanceId;
            if (ringId == -1 || activeRingIds.find(ringId) == activeRingIds.end()) {
                states[i].isInRing = false;
                states[i].ringSize = 0;
                states[i].ringInstanceId = -1;
                states[i].cycleBondId = -1;
            }
        }
    }
}

// ============================================================================
// HELPER: Apply Coulomb Forces (Electromagnetic O(N))
// ============================================================================
void PhysicsEngine::applyCoulombForces(float dt,
                                       std::vector<TransformComponent>& transforms,
                                       const std::vector<AtomComponent>& atoms,
                                       const ChemistryDatabase& db) {
    for (int i = 0; i < (int)transforms.size(); i++) {
        float q1 = atoms[i].partialCharge;
        if (std::abs(q1) < Config::CHARGE_THRESHOLD) continue;

        std::vector<int> neighbors = grid.getNearby({transforms[i].x, transforms[i].y}, Config::EM_REACH);
        for (int j : neighbors) {
            if (i == j) continue;
            float q2 = atoms[j].partialCharge;
            if (std::abs(q2) < Config::CHARGE_THRESHOLD) continue;

            float distSq = MathUtils::distSq(transforms[i].x, transforms[i].y, transforms[j].x, transforms[j].y);
            if (distSq > Config::EM_REACH * Config::EM_REACH) continue;
            
            float dist = std::sqrt(distSq + (Config::PHYSICS_EPSILON * Config::PHYSICS_EPSILON));
            if (dist > Config::EM_REACH) continue;

            // Coulomb's Law: F = k * (q1 * q2) / r^2
            float effectiveDist = std::max(dist, Config::MIN_COULOMB_DIST);
            float forceMag = (Config::COULOMB_CONSTANT * q1 * q2) / (effectiveDist * effectiveDist);
            
            float dx = transforms[j].x - transforms[i].x;
            float dy = transforms[j].y - transforms[i].y;
            
            Vector3 diff = {dx, dy, 0.0f};
            Vector3 dir = MathUtils::safeNormalize(diff);
            
            float fx = dir.x * forceMag;
            float fy = dir.y * forceMag;

            float m1 = db.getElement(atoms[i].atomicNumber).atomicMass;
            float m2 = db.getElement(atoms[j].atomicNumber).atomicMass;
            
            if (m1 < 0.01f) m1 = 1.0f;
            if (m2 < 0.01f) m2 = 1.0f;

            // Player force clamping
            if (i == 0) { 
                float maxF = 150.0f; 
                fx = std::clamp(fx, -maxF, maxF);
                fy = std::clamp(fy, -maxF, maxF);
            }

            transforms[i].vx -= (fx / m1) * dt;
            transforms[i].vy -= (fy / m1) * dt;
            transforms[j].vx += (fx / m2) * dt;
            transforms[j].vy += (fy / m2) * dt;
            
            // Clamp Coulomb speed
            constexpr float MAX_COULOMB_SPEED = 600.0f;
            MathUtils::ClampMagnitude(transforms[i].vx, transforms[i].vy, MAX_COULOMB_SPEED);
            MathUtils::ClampMagnitude(transforms[j].vx, transforms[j].vy, MAX_COULOMB_SPEED);
        }
    }
}

// ============================================================================
// HELPER: Apply Bond Springs (Elastic Bonds & Molecular Stress)
// ============================================================================
void PhysicsEngine::applyBondSprings(float dt,
                                     std::vector<TransformComponent>& transforms,
                                     const std::vector<AtomComponent>& atoms,
                                     std::vector<StateComponent>& states,
                                     const ChemistryDatabase& db,
                                     int diagCounter) {
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (!states[i].isClustered || states[i].parentEntityId == -1) continue;

        int parentId = states[i].parentEntityId;
        int slotIdx = states[i].parentSlotIndex;

        const Element& parentElem = db.getElement(atoms[parentId].atomicNumber);
        // BOUNDS CHECK: Skip if slotIdx invalid or bondingSlots empty
        if (slotIdx < 0 || parentElem.bondingSlots.empty() || 
            slotIdx >= (int)parentElem.bondingSlots.size()) continue;

        Vector3 slotDir = parentElem.bondingSlots[slotIdx];
        
        float targetX = transforms[parentId].x + slotDir.x * Config::BOND_IDEAL_DIST;
        float targetY = transforms[parentId].y + slotDir.y * Config::BOND_IDEAL_DIST;
        float targetZ = transforms[parentId].z + slotDir.z * Config::BOND_IDEAL_DIST;

        float dx = targetX - transforms[i].x;
        float dy = targetY - transforms[i].y;
        float dz = targetZ - transforms[i].z;
        float dist = MathUtils::length(dx, dy, dz);

        // Stress breakup for non-player molecules
        bool isPlayerMolecule = (states[i].moleculeId == 0 || i == 0 || parentId == 0);
        
        if (!isPlayerMolecule && dist > Config::BOND_BREAK_STRESS) {
            if (states[i].cycleBondId != -1 || states[i].isInRing) {
                int ringId = states[i].ringInstanceId;
                RingChemistry::invalidateRing(ringId, states);
            }

            states[i].isClustered = false;
            states[i].parentEntityId = -1;
            
            TraceLog(LOG_WARNING, "[PHYSICS] BOND BROKEN by stress: Atom %d separated from %d", i, (int)parentId);
            continue;
        }

        // Ring vs Normal bond physics
        float fx, fy, fz;
        
        // SKIP SPRINGS DURING DOCKING ANIMATION - let StructuralPhysics control
        if (states[i].isInRing && states[i].dockingProgress < 1.0f) {
            continue;  // Don't apply conflicting bond spring forces
        }
        
        if (states[i].isInRing && states[parentId].isInRing) {
            // Ring bonds: distance-only forces
            float actualDx = transforms[parentId].x - transforms[i].x;
            float actualDy = transforms[parentId].y - transforms[i].y;
            float actualDz = transforms[parentId].z - transforms[i].z;
            float actualDist = MathUtils::length(actualDx, actualDy, actualDz);
            
            if (actualDist > 0.1f) {
                float strain = actualDist - Config::BOND_IDEAL_DIST;
                float ringSpringK = Config::BOND_SPRING_K * Config::Physics::RING_SPRING_MULTIPLIER; 
                float forceMag = strain * ringSpringK;
                
                float nx = actualDx / actualDist;
                float ny = actualDy / actualDist;
                float nz = actualDz / actualDist;
                
                fx = nx * forceMag;
                fy = ny * forceMag;
                fz = nz * forceMag;
                
                fx = std::clamp(fx, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
                fy = std::clamp(fy, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
                fz = std::clamp(fz, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
            } else {
                fx = fy = fz = 0;
            }
        } else {
            // Normal bonds: VSEPR slot direction (Hooke's Law)
            fx = dx * Config::BOND_SPRING_K;
            fy = dy * Config::BOND_SPRING_K;
            fz = dz * Config::BOND_SPRING_K;
            
            fx = std::clamp(fx, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
            fy = std::clamp(fy, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
            fz = std::clamp(fz, -Config::MAX_SPRING_FORCE, Config::MAX_SPRING_FORCE);
        }

        float m1 = db.getElement(atoms[i].atomicNumber).atomicMass;
        float mP = parentElem.atomicMass;
        if (m1 < 0.01f) m1 = 1.0f;
        if (mP < 0.01f) mP = 1.0f;

        // Apply to both (Action and Reaction)
        transforms[i].vx += (fx / m1) * dt;
        transforms[i].vy += (fy / m1) * dt;
        transforms[i].vz += (fz / m1) * dt;
        
        transforms[parentId].vx -= (fx / mP) * dt;
        transforms[parentId].vy -= (fy / mP) * dt;
        transforms[parentId].vz -= (fz / mP) * dt;
        
        // Stress diagnostics (every 2 seconds)
        if (diagCounter > 120) {
             if (states[parentId].moleculeId == 0) {
                 float strain = (dist - Config::BOND_IDEAL_DIST);
                 if (abs(strain) > 5.0f) {
                     TraceLog(LOG_INFO, "[STRESS] Bond %d->%d (Slot %d) | Dist: %.1f / %.1f | Strain: %.1f", 
                              parentId, i, slotIdx, dist, Config::BOND_IDEAL_DIST, strain);
                 }
             }
        }
    }
}

// ============================================================================
// HELPER: Apply Cycle Bonds (Non-Hierarchical Ring Springs)
// ============================================================================
void PhysicsEngine::applyCycleBonds(float dt,
                                    std::vector<TransformComponent>& transforms,
                                    const std::vector<AtomComponent>& atoms,
                                    const std::vector<StateComponent>& states,
                                    const ChemistryDatabase& db) {
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (states[i].cycleBondId == -1) continue;

        int partnerId = states[i].cycleBondId;
        if (i > partnerId) continue; // Avoid double processing

        float dx = transforms[partnerId].x - transforms[i].x;
        float dy = transforms[partnerId].y - transforms[i].y;
        float dz = transforms[partnerId].z - transforms[i].z;
        float dist = MathUtils::length(dx, dy, dz);

        if (dist < 0.1f) continue;

        float strain = dist - Config::BOND_IDEAL_DIST;
        float ringSpringK = Config::BOND_SPRING_K * Config::Physics::RING_SPRING_MULTIPLIER; 
        float forceMag = strain * ringSpringK;
        
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;
        
        float fx = nx * forceMag;
        float fy = ny * forceMag;
        float fz = nz * forceMag;
        
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
}

// ============================================================================
// HELPER: Integrate Motion (Velocity/Position + Friction + Boundaries)
// ============================================================================
void PhysicsEngine::integrateMotion(float dt,
                                    std::vector<TransformComponent>& transforms,
                                    const std::vector<StateComponent>& states) {
    for (size_t idx = 0; idx < transforms.size(); idx++) {
        TransformComponent& tr = transforms[idx];
        
        // Integration with thermodynamic jitter
        float jitterX = MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
        float jitterY = MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
        float jitterZ = MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER * 0.2f;

        tr.vx += jitterX * dt;
        tr.vy += jitterY * dt;
        tr.vz += jitterZ * dt;

        tr.x += tr.vx * dt;
        tr.y += tr.vy * dt;
        tr.z += tr.vz * dt;

        // Hard snap Z=0 for established rings
        if (states[idx].isInRing && states[idx].isLocked()) {
            tr.z = 0.0f;
            tr.vz = 0.0f;
        }

        // Ambient friction
        tr.vx *= Config::DRAG_COEFFICIENT;
        tr.vy *= Config::DRAG_COEFFICIENT;
        tr.vz *= Config::DRAG_COEFFICIENT;

        // World boundaries (Z)
        if (tr.z < Config::WORLD_DEPTH_MIN) {
            tr.z = Config::WORLD_DEPTH_MIN;
            tr.vz *= Config::WORLD_BOUNCE;
        } else if (tr.z > Config::WORLD_DEPTH_MAX) {
            tr.z = Config::WORLD_DEPTH_MAX;
            tr.vz *= Config::WORLD_BOUNCE;
        }
    }
}

// ============================================================================
// MAIN STEP: Orchestrates all physics subsystems
// ============================================================================
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
    
    static int diagCounter = 0;
    
    // 0. Update environment
    environment.update(transforms, states, dt); 

    // 0.5 Precalculate roots (Phase 28 Optimization)
    std::vector<int> rootCache(transforms.size());
    for (int i = 0; i < (int)transforms.size(); i++) {
        rootCache[i] = MathUtils::findMoleculeRoot(i, states);
    }

    // 0.6 Ring integrity validation
    validateRingIntegrity(states);

    // 1. Electromagnetic forces (Coulomb)
    applyCoulombForces(dt, transforms, atoms, db);

    // 2. Elastic bonds and molecular stress
    applyBondSprings(dt, transforms, atoms, states, db, diagCounter);

    // 3. Cycle bonds (non-hierarchical ring springs)
    applyCycleBonds(dt, transforms, atoms, states, db);

    // 4. Structural dynamics (rings & rigid groups)
    StructuralPhysics::applyRingDynamics(dt, transforms, atoms, states, rootCache);

    // 5. Folding & affinity (catalytic synthesis)
    StructuralPhysics::applyFoldingAndAffinity(dt, transforms, atoms, states, environment, rootCache);

    // 6. Spontaneous bonding (autonomous evolution)
    BondingSystem::updateSpontaneousBonding(states, atoms, transforms, grid, rootCache, &environment, tractedEntityId);

    // 7. Integration, friction, and boundaries
    integrateMotion(dt, transforms, states);

    // 8. Update spatial grid
    diagCounter++;
    if (diagCounter > 120) diagCounter = 0;
    grid.update(transforms);

    // 9. Reset frame-local flags
    for (auto& s : states) s.justBonded = false;
}
