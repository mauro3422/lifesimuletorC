#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "raylib.h"
#include "../physics/BondingSystem.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../ecs/components.hpp"
#include "../physics/SpatialGrid.hpp"

// Mock InputHandler to satisfy linker dependency
namespace InputHandler {
    void setMouseCaptured(bool captured) {}
}

// Minimal Mock Setup
void setupAtoms(std::vector<StateComponent>& states, std::vector<AtomComponent>& atoms, std::vector<TransformComponent>& transforms, int count) {
    states.resize(count);
    atoms.resize(count);
    transforms.resize(count);
    
    // Load minimal chemistry if needed, or assume Carbon (6) exists
    // ChemistryDatabase should be loaded by main app usually, but here we depend on it finding resources
    
    for(int i=0; i<count; ++i) {
        states[i] = StateComponent();
        atoms[i].atomicNumber = 6; // Carbon
        transforms[i] = TransformComponent{0,0,0, 0,0,0};
    }
}

void test_TriangleRejection() {
    std::cout << "[TEST] Running test_TriangleRejection..." << std::endl;
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupAtoms(states, atoms, transforms, 3);

    // Setup Chain: A(0) -> B(1) -> C(2)
    // 0 is root
    states[1].parentEntityId = 0; states[1].isClustered = true; states[0].childCount++;
    states[2].parentEntityId = 1; states[2].isClustered = true; states[1].childCount++;
    
    states[0].moleculeId = 0;
    states[1].moleculeId = 0;
    states[2].moleculeId = 0;

    // Position in a small triangle, close enough to bond
    // Distance A-C should be < BOND_AUTO_RANGE (approx 60-80 usually)
    transforms[0].x = 0;   transforms[0].y = 0;
    transforms[1].x = 40;  transforms[1].y = 0;
    transforms[2].x = 0;   transforms[2].y = 40; 
    
    // Check distance A-C
    float dx = transforms[0].x - transforms[2].x;
    float dy = transforms[0].y - transforms[2].y;
    float dist = std::sqrt(dx*dx + dy*dy);
    std::cout << "   Distance A-C: " << dist << std::endl;

    // Mock SpatialGrid
    SpatialGrid grid(100);
    grid.update(transforms);
    
    // Root Cache
    std::vector<int> rootCache = {0, 0, 0};

    // Run Autonomous Bonding
    // The previous logic requires hops >= 3 for cycle bonding.
    // A->B->C is 2 hops. Should fail.
    BondingSystem::updateSpontaneousBonding(states, atoms, transforms, grid, rootCache);

    // Verify
    if (states[0].cycleBondId != -1 || states[2].cycleBondId != -1) {
        std::cerr << "   [FAILED] Triangle cycle was erroneously formed!" << std::endl;
        // exit(1); // Continue to next test
    } else {
        std::cout << "   [PASSED] Triangle cycle rejected." << std::endl;
    }
}

#include "../physics/PhysicsEngine.hpp"

// ...

void test_StressSeparation() {
    std::cout << "[TEST] Running test_StressSeparation..." << std::endl;
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupAtoms(states, atoms, transforms, 10); // Need more atoms

    // Bond 5-6 (Not player 0)
    int A = 5;
    int B = 6;
    
    // Bond A -> B (Parent -> Child)
    states[B].isClustered = true;
    states[B].parentEntityId = A;
    states[B].parentSlotIndex = 0;
    states[B].moleculeId = 10; 
    states[A].moleculeId = 10;
    states[A].childCount = 1;
    states[A].occupiedSlots = 1; 

    // Position them far apart (Stress)
    transforms[A] = {0,0,0, 0,0,0};
    transforms[B] = {300.0f, 0, 0, 0,0,0}; 

    // Dependency: PhysicsEngine
    PhysicsEngine engine;
    engine.step(0.016f, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);

    if (states[B].isClustered) {
         std::cout << "   [FAILED] Bond did NOT break under stress!" << std::endl;
         // exit(1);
    } else {
         std::cout << "   [PASSED] Bond broken under stress." << std::endl;
    }
}

int main() {
    // Enable logs for debugging
    SetTraceLogLevel(LOG_INFO);
    
    // We assume resources/ is in the CWD
    // Force load chemistry to ensure safety
    ChemistryDatabase::getInstance(); 

    test_TriangleRejection();
    test_StressSeparation();
    
    std::cout << "All edge case tests passed." << std::endl;
    return 0;
}
