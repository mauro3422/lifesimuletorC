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
    static int diagCounter = 0;
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
                float ringSpringK = 6.0f; // Gentle - just maintain, don't pull hard
                float forceMag = strain * ringSpringK;
                
                // Normalize direction
                float nx = actualDx / actualDist;
                float ny = actualDy / actualDist;
                float nz = actualDz / actualDist;
                
                fx = nx * forceMag;
                fy = ny * forceMag;
                fz = nz * forceMag;
            } else {
                fx = fy = fz = 0;
            }
        } else {
            // NORMAL BONDS: Use VSEPR slot direction (original behavior)
            // --- HOOKE'S LAW (Restoration Force) with Z ---
            fx = dx * Config::BOND_SPRING_K;
            fy = dy * Config::BOND_SPRING_K;
            fz = dz * Config::BOND_SPRING_K;
        }

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

        // Use SAME gentle spring as other ring bonds (distance-based)
        float strain = dist - Config::BOND_IDEAL_DIST;
        float ringSpringK = 6.0f; // Same as other ring bonds
        float forceMag = strain * ringSpringK;
        
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;
        
        float fx = nx * forceMag;
        float fy = ny * forceMag;
        float fz = nz * forceMag;

        float m1 = ChemistryDatabase::getInstance().getElement(atoms[i].atomicNumber).atomicMass;
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

    // --- PHASE 23: RING STABILITY (Keep ring shape) ---
    // Angular forces were causing oscillation - instead, just use very strong damping
    // Since the ring is positioned correctly at formation time, we just need to keep it stable
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (!states[i].isInRing) continue;
        
        // --- Z-AXIS FLATTENING: Keep all ring atoms on the same plane ---
        transforms[i].vz -= transforms[i].z * 20.0f * dt;
        transforms[i].vz *= 0.5f;
        
        // FREEZE ring atoms almost completely
        // The square was positioned correctly at formation - just maintain it
        float ringDamping = 0.30f; // Very heavy damping - almost frozen
        transforms[i].vx *= ringDamping;
        transforms[i].vy *= ringDamping;
    }

    // --- PHASE 32: ACTIVE RING FOLDING (Clay Catalysis) ---
    // Find TERMINAL atoms (exactly 1 bond) in chains on Clay and pull them together.
    // A terminal atom has: (parent but no children) OR (children but no parent).
    // We collect all terminals per molecule, then apply folding force between them.
    
    // PHASE 36: CARBON AFFINITY - Isolated/terminal carbons seek each other
    // This creates "intelligent" chain formation where carbons actively find partners
    std::vector<int> seekingCarbons; // Carbons that need bonds
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (states[i].isInRing) continue; // Skip ALL ring atoms, not just those with cycleBondId
        if (atoms[i].atomicNumber != 6) continue; // Only Carbon
        
        // Check if on Clay
        float rangeMultiplier = environment.getBondRangeMultiplier({transforms[i].x, transforms[i].y});
        if (rangeMultiplier < 1.2f) continue;
        
        // Count bonds
        int bondCount = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        
        // Carbons with available valency (less than 4 bonds) can seek partners
        if (bondCount < 4) {
            seekingCarbons.push_back(i);
        }
    }
    
    // Apply attraction between seeking carbons
    for (size_t a = 0; a < seekingCarbons.size(); a++) {
        for (size_t b = a + 1; b < seekingCarbons.size(); b++) {
            int c1 = seekingCarbons[a];
            int c2 = seekingCarbons[b];
            
            float dx = transforms[c2].x - transforms[c1].x;
            float dy = transforms[c2].y - transforms[c1].y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            // Only attract if within reasonable range but not too close
            if (dist > 30.0f && dist < 150.0f) {
                // Check if different molecules - this encourages chain GROWTH
                int root1 = MathUtils::findMoleculeRoot(c1, states);
                int root2 = MathUtils::findMoleculeRoot(c2, states);
                
                float affinityStrength = (root1 != root2) ? 15.0f : 10.0f; // Stronger for chain building
                float nx = dx / dist;
                float ny = dy / dist;
                
                transforms[c1].vx += nx * affinityStrength * dt;
                transforms[c1].vy += ny * affinityStrength * dt;
                transforms[c2].vx -= nx * affinityStrength * dt;
                transforms[c2].vy -= ny * affinityStrength * dt;
            }
        }
    }
    
    // Now collect terminals for RING CLOSING (same molecule only)
    std::vector<int> terminals; // Terminal atoms on Clay
    for (int i = 0; i < (int)transforms.size(); i++) {
        if (states[i].isInRing) continue; // Skip ALL ring atoms
        
        // Check if on Clay
        float rangeMultiplier = environment.getBondRangeMultiplier({transforms[i].x, transforms[i].y});
        if (rangeMultiplier < 1.2f) continue;
        
        // Count bonds: parent counts as 1, each child counts as 1
        int bondCount = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        
        // Terminal = exactly 1 bond (end of chain)
        if (bondCount == 1) {
            terminals.push_back(i);
        }
    }
    
    // For each pair of terminals, if they're in the same molecule, pull together
    for (size_t a = 0; a < terminals.size(); a++) {
        for (size_t b = a + 1; b < terminals.size(); b++) {
            int t1 = terminals[a];
            int t2 = terminals[b];
            
            // Check if same molecule
            int root1 = MathUtils::findMoleculeRoot(t1, states);
            int root2 = MathUtils::findMoleculeRoot(t2, states);
            if (root1 != root2) continue; // Different molecules
            
            // Already close enough to bond? Skip (let BondingSystem handle it)
            float dx = transforms[t2].x - transforms[t1].x;
            float dy = transforms[t2].y - transforms[t1].y;
            float dz = transforms[t2].z - transforms[t1].z;
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist > 20.0f && dist < 300.0f) {
                // Apply magnetic folding force - pull terminals toward each other
                float foldingStrength = 18.0f; // Reduced from 25.0 to prevent oscillation
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
                
                // Pull t1 toward t2
                transforms[t1].vx += nx * foldingStrength * dt;
                transforms[t1].vy += ny * foldingStrength * dt;
                transforms[t1].vz += nz * foldingStrength * dt;
                
                // Pull t2 toward t1
                transforms[t2].vx -= nx * foldingStrength * dt;
                transforms[t2].vy -= ny * foldingStrength * dt;
                transforms[t2].vz -= nz * foldingStrength * dt;
            }
        }
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
    diagCounter++;
    if (diagCounter > 120) diagCounter = 0;
    grid.update(transforms);
}
