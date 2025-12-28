/**
 * test_ladder_diag.cpp
 * Diagnostic script to verify the 3D positioning and alignment of carbon ladders.
 * Logs coordinates and bond distances to identify clustering issues.
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

#include "raylib.h"
#include "../ecs/components.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "../physics/BondingSystem.hpp"
#include "../chemistry/ChemistryDatabase.hpp"

void printAtoms(const std::vector<TransformComponent>& tr, const std::vector<StateComponent>& st) {
    std::cout << std::left << std::setw(5) << "ID" 
              << std::setw(10) << "X" << std::setw(10) << "Y" << std::setw(10) << "Z" 
              << std::setw(10) << "InRing" << std::setw(10) << "RingID" << std::endl;
    for(size_t i=1; i<tr.size(); i++) {
        std::cout << std::left << std::setw(5) << i 
                  << std::fixed << std::setprecision(1)
                  << std::setw(10) << tr[i].x << std::setw(10) << tr[i].y << std::setw(10) << tr[i].z
                  << std::setw(10) << (st[i].isInRing ? "YES" : "NO") 
                  << std::setw(10) << st[i].ringInstanceId << std::endl;
    }
}

int main() {
    std::cout << "=== LADDER GEOMETRY DIAGNOSTIC ===" << std::endl;
    
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    PhysicsEngine physics;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // 1. DUMMY PLAYER
    transforms.push_back({0,0,0,0,0,0,0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, true});

    // 2. Spawn 8 Carbons in a rough ladder shape
    // Square 1: Bottom (1,2,3,4)
    float base_x = 150, base_y = 150; // In Clay Zone
    float d = Config::BOND_IDEAL_DIST;
    
    // Atoms 1,2,3,4 (Square 1) - Pre-bonded for start
    transforms.push_back({base_x, base_y, 0, 0,0,0,0}); atoms.push_back({6,0}); states.push_back({true, 1, -1, -1, 1.0f, false, 0, 0, -1, true, 4, 1, 1, false}); // 1. Root
    transforms.push_back({base_x + d, base_y, 0, 0,0,0,0}); atoms.push_back({6,0}); states.push_back({true, 1, 1, 0, 1.0f, false, 0, 0, -1, true, 4, 1, 1, false}); // 2. Child of 1
    transforms.push_back({base_x + d, base_y + d, 0, 0,0,0,0}); atoms.push_back({6,0}); states.push_back({true, 1, 2, 0, 1.0f, false, 0, 0, -1, true, 4, 2, 1, false}); // 3. Child of 2
    transforms.push_back({base_x, base_y + d, 0, 0,0,0,0}); atoms.push_back({6,0}); states.push_back({true, 1, 3, 0, 1.0f, false, 0, 0, -1, true, 4, 3, 1, false}); // 4. Child of 3
    states[3].cycleBondId = 1; states[1].cycleBondId = 3; // Close cycle 1

    // Atoms 5, 6 (Attached to 1 and 2 to form next square)
    // Positioned slightly away to trigger alignment
    transforms.push_back({base_x - 30, base_y - 20, 0, 0,0,0,0}); atoms.push_back({6,0}); states.push_back({false, -1, 1, 1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false}); // 5. Child of 1
    transforms.push_back({base_x + d + 30, base_y - 20, 0, 0,0,0,0}); atoms.push_back({6,0}); states.push_back({false, -1, 2, 1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false}); // 6. Child of 2

    std::cout << "INITIAL STATE:" << std::endl;
    printAtoms(transforms, states);

    // Run Simulation
    const int STEPS = 500;
    for(int i=0; i<STEPS; i++) {
        physics.step(0.016f, transforms, atoms, states, db, -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
        
        if (i % 100 == 0) {
            std::cout << "\nSTEP " << i << ":" << std::endl;
            // Check distances for atoms 5 and 6 relative to their parents
            float d51 = MathUtils::dist(transforms[5].x, transforms[5].y, transforms[1].x, transforms[1].y);
            float d62 = MathUtils::dist(transforms[6].x, transforms[6].y, transforms[2].x, transforms[2].y);
            std::cout << "Dist 5-1: " << d51 << ", Dist 6-2: " << d62 << std::endl;
        }
    }

    std::cout << "\nFINAL STATE:" << std::endl;
    printAtoms(transforms, states);
    
    // Analyze Square 2 formation
    if (states[5].isInRing && states[6].isInRing) {
        std::cout << "[SUCCESS] Second ring formed!" << std::endl;
    } else {
        std::cout << "[FAIL] Atoms 5 and 6 did not form a second ring." << std::endl;
    }

    return 0;
}
