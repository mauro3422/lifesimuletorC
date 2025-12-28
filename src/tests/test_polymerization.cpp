#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "../physics/BondingSystem.hpp"
#include "../ecs/World.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"

// Simple Test Framework
void assert_true(bool condition, const std::string& msg) {
    if (!condition) {
        std::cerr << "[FAIL] " << msg << std::endl;
    } else {
        std::cout << "[PASS] " << msg << std::endl;
    }
}

void run_square_formation_test() {
    std::cout << "\n=== TEST: Spontaneous C4 Square Formation (Clay Zone) ===" << std::endl;
    World world;
    PhysicsEngine physics;
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");

    // DUMMY PLAYER (Index 0)
    world.transforms.push_back({0,0,0,0,0,0,0});
    world.atoms.push_back({1, 0.0f});
    world.states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, true});

    // Spawn 4 Carbons
    float cx = -1200, cy = -400;
    std::vector<Vector3> offsets = {{-18,-18}, {18,-18}, {18,18}, {-18,18}};
    
    // Atoms 1, 2, 3, 4
    for(int i=0; i<4; i++) {
        world.transforms.push_back({cx + offsets[i].x, cy + offsets[i].y, 0, 0,0,0, 0});
        world.atoms.push_back({6, 0.0f}); // Carbon
        world.states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
    }

    // Run Simulation
    const int FRAMES = 1200; 
    for(int f=0; f<FRAMES; f++) {
        physics.step(0.016f, world.transforms, world.atoms, world.states, db, -1);
        BondingSystem::updateHierarchy(world.transforms, world.states, world.atoms);
    }

    // Verify
    int ringCount = 0;
    for(size_t i=1; i<world.states.size(); i++) {
        if (world.states[i].isInRing) ringCount++;
    }
    
    std::cout << "Atoms in Ring: " << ringCount << "/4" << std::endl;
    assert_true(ringCount == 4, "All 4 atoms should form a ring");
    
    int badBonds = 0;
    for(size_t i=1; i<world.states.size(); i++) {
        int parent = world.states[i].parentEntityId;
        if (parent != -1) {
             float d = MathUtils::dist(world.transforms[i].x, world.transforms[i].y, 
                                     world.transforms[parent].x, world.transforms[parent].y);
             if (d > Config::BOND_IDEAL_DIST * 1.3f) badBonds++;
        }
    }
    std::cout << "Bad/Stretched Bonds: " << badBonds << std::endl;
    assert_true(badBonds == 0, "Structure should be relaxed (Square)");
}

void run_stacking_test() {
    std::cout << "\n=== TEST: Carbon Ladder Stacking (Macro-Alignment) ===" << std::endl;
    World world;
    PhysicsEngine physics;
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    
    // DUMMY PLAYER (Index 0)
    world.transforms.push_back({0,0,0,0,0,0,0});
    world.atoms.push_back({1, 0.0f});
    world.states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, true});

    // Spawn 2 Pre-formed Squares in Clay Zone
    float x1 = -1200;
    float y1 = -400;
    
    // Ring 1 (Bottom) - Indices 1,2,3,4. Parents relative to 0 start.
    world.transforms.push_back({x1 - 25, y1 - 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 1, -1, -1, 1.0f, false, 0, 0, -1, true, 4, 0, 1, false}); // ID 1. Root.
    world.transforms.push_back({x1 + 25, y1 - 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 1, 1, 0, 1.0f, false, 0, 0, -1, true, 4, 1, 1, false}); // ID 2. Parent 1.
    world.transforms.push_back({x1 + 25, y1 + 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 1, 2, 0, 1.0f, false, 0, 0, -1, true, 4, 2, 1, false}); // ID 3. Parent 2.
    world.transforms.push_back({x1 - 25, y1 + 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 1, 3, 0, 1.0f, false, 0, 0, -1, true, 4, 3, 1, false}); // ID 4. Parent 3.

    // Ring 2 (Top) - Indices 5,6,7,8
    float x2 = -1200;
    float y2 = y1 - 80; 
    world.transforms.push_back({x2 - 25, y2 - 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 2, -1, -1, 1.0f, false, 0, 0, -1, true, 4, 0, 2, false}); // ID 5. Root.
    world.transforms.push_back({x2 + 25, y2 - 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 2, 5, 0, 1.0f, false, 0, 0, -1, true, 4, 1, 2, false}); // ID 6. Parent 5.
    world.transforms.push_back({x2 + 25, y2 + 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 2, 6, 0, 1.0f, false, 0, 0, -1, true, 4, 2, 2, false}); // ID 7. Parent 6.
    world.transforms.push_back({x2 - 25, y2 + 25, 0, 0,0,0, 0}); world.atoms.push_back({6, 0}); world.states.push_back({true, 2, 7, 0, 1.0f, false, 0, 0, -1, true, 4, 3, 2, false}); // ID 8. Parent 7.

    // Run Simulation
    const int FRAMES = 1200;
    for(int f=0; f<FRAMES; f++) {
        // Update Root Cache manually if physics doesn't? Physics step DOES update it.
        physics.step(0.016f, world.transforms, world.atoms, world.states, db, -1);
    }
    
    float finalDist = MathUtils::dist(world.transforms[1].x, world.transforms[1].y, world.transforms[5].x, world.transforms[5].y);
    std::cout << "Final Distance between rings: " << finalDist << " (Start: 80)" << std::endl;
    
    if (finalDist < 75.0f) {
        std::cout << "[PASS] Rings attracted each other." << std::endl;
    } else {
        std::cout << "[FAIL] Rings didn't attract. Magnetism failed." << std::endl;
    }
}

int main() {
    std::cout << "Running Polymerization Tests (Fixed Indices)..." << std::endl;
    run_square_formation_test();
    run_stacking_test();
    return 0;
}
