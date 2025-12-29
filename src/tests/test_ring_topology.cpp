/**
 * test_ring_topology.cpp
 * Validates that ring formation creates correct member ordering and positions
 * for hexagonal carbon rings (C6).
 * 
 * Key Tests:
 * 1. Triangles (3-atom) are rejected
 * 2. Valid 6-ring hexagon formation
 * 3. All ring members marked as isInRing
 * 4. Ring size correctly calculated (6)
 * 5. Hexagon positions are correct
 * 6. Z-axis flattened to 0
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#include "raylib.h"

#include "../ecs/components.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"

#include "../physics/RingChemistry.hpp"

#define TEST(name) std::cout << "[TEST] " << #name << "... "; testsRun++;
#define PASS std::cout << "PASS" << std::endl; testsPassed++;
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl;

int testsRun = 0;
int testsPassed = 0;

/**
 * Helper: Create a chain of 6 carbon atoms for hexagon formation
 * Hierarchy: 0 -> 1 -> 2 -> 3 -> 4, and 0 -> 5
 * This forms a "Y" shape where cycle bond 4-5 creates a 6-ring
 */
void setupHexagonChain(std::vector<TransformComponent>& transforms,
                       std::vector<AtomComponent>& atoms,
                       std::vector<StateComponent>& states) {
    transforms.clear();
    atoms.clear();
    states.clear();
    
    // Create 6 carbon atoms in a chain-like hierarchy
    float spacing = 30.0f;
    float angle = 2.0944f / 2.0f; // 60 degrees for hexagon setup
    
    for (int i = 0; i < 6; i++) {
        // Position in a rough pre-hexagon shape
        float x = std::cos(i * 1.0472f) * 50.0f; // 60 degree spacing
        float y = std::sin(i * 1.0472f) * 50.0f;
        transforms.push_back({x, y, 0.0f, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});  // Carbon
        
        StateComponent state = {};
        state.isClustered = (i > 0); // First atom is root
        state.moleculeId = 1;
        state.parentEntityId = (i > 0) ? (i == 5 ? 0 : i - 1) : -1; // 5 -> 0, others -> prev
        state.parentSlotIndex = 0;
        state.dockingProgress = 1.0f;
        state.childCount = (i == 0) ? 2 : (i < 4) ? 1 : 0;  // Root has 2 children, middle have 1
        state.cycleBondId = -1;
        state.isInRing = false;
        state.ringSize = 0;
        state.ringInstanceId = -1;
        state.ringIndex = -1;
        states.push_back(state);
    }
}

/**
 * Helper: Check if two atoms are bonded (hierarchically or via cycle)
 */
bool areBonded(int a, int b, const std::vector<StateComponent>& states) {
    if (states[a].parentEntityId == b || states[b].parentEntityId == a) {
        return true;
    }
    if (states[a].cycleBondId == b || states[b].cycleBondId == a) {
        return true;
    }
    return false;
}

int main() {
    std::cout << "=== RING TOPOLOGY TESTS (HEXAGON) ===" << std::endl << std::endl;
    
    ChemistryDatabase::getInstance().reload();
    // StructureRegistry auto-loads via getInstance()
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // --------------------------------------------
    // TEST 1: Triangle (3-atom) rejection
    // --------------------------------------------
    TEST(Triangle_Rejection) {
        transforms.clear(); atoms.clear(); states.clear();
        
        // Simple 3-atom chain: A -> B -> C, try to close C-A
        transforms.push_back({0, 0, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0}); 
        states.push_back({true, 1, -1, -1, 1.0f, false, 1, 1, -1, false, 0, -1, -1, false});
        
        transforms.push_back({30, 0, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0}); 
        states.push_back({true, 1, 0, 0, 1.0f, false, 1, 1, -1, false, 0, -1, -1, false});
        
        transforms.push_back({60, 0, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0}); 
        states.push_back({true, 1, 1, 0, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
        
        BondError result = RingChemistry::tryCycleBond(2, 0, states, atoms, transforms);
        
        if (result == BondError::RING_TOO_SMALL) {
            PASS
        } else {
            FAIL("Expected RING_TOO_SMALL, got different error")
        }
    }
    
    // --------------------------------------------
    // TEST 2: Valid 6-ring hexagon formation
    // --------------------------------------------
    TEST(Valid_6Ring_Formation) {
        setupHexagonChain(transforms, atoms, states);
        
        // Try to form cycle bond between atom 4 and atom 5
        // Path: 4 -> 3 -> 2 -> 1 -> 0 <- 5
        // This creates a 6-ring: 0-1-2-3-4-5-0
        BondError result = RingChemistry::tryCycleBond(4, 5, states, atoms, transforms);
        
        if (result == BondError::SUCCESS) {
            // Verify cycle bonds were set
            if (states[4].cycleBondId == 5 && states[5].cycleBondId == 4) {
                PASS
            } else {
                FAIL("Cycle bond IDs not set correctly")
            }
        } else {
            FAIL("Expected SUCCESS for 6-ring formation")
        }
    }
    
    // --------------------------------------------
    // TEST 3: All ring members marked as isInRing
    // --------------------------------------------
    TEST(All_Members_In_Ring) {
        bool allInRing = true;
        for (int i = 0; i < 6; i++) {
            if (!states[i].isInRing) {
                allInRing = false;
                std::cout << "Atom " << i << " not in ring! " << std::endl;
            }
        }
        if (allInRing) {
            PASS
        } else {
            FAIL("Not all atoms marked as isInRing")
        }
    }
    
    // --------------------------------------------
    // TEST 4: Ring size correctly calculated
    // --------------------------------------------
    TEST(Ring_Size_Correct) {
        bool sizeCorrect = true;
        for (int i = 0; i < 6; i++) {
            if (states[i].ringSize != 6) {
                sizeCorrect = false;
                std::cout << "Atom " << i << " ringSize: " << states[i].ringSize << std::endl;
            }
        }
        if (sizeCorrect) {
            PASS
        } else {
            FAIL("Ring size not 6 for all atoms")
        }
    }
    
    // --------------------------------------------
    // TEST 5: Hexagon positions form regular hexagon
    // --------------------------------------------
    TEST(Hexagon_Positions) {
        // Calculate centroid
        float cx = 0, cy = 0;
        for (int i = 0; i < 6; i++) {
            cx += transforms[i].x;
            cy += transforms[i].y;
        }
        cx /= 6; cy /= 6;
        
        // For a regular hexagon with side = BOND_IDEAL_DIST:
        // Circumradius R = side / (2 * sin(PI/6)) = side / (2 * 0.5) = side
        float expectedRadius = Config::BOND_IDEAL_DIST / (2.0f * std::sin(3.14159f / 6.0f));
        
        bool allAtVertices = true;
        for (int i = 0; i < 6; i++) {
            float dist = std::sqrt(std::pow(transforms[i].x - cx, 2) + 
                                   std::pow(transforms[i].y - cy, 2));
            // Allow 5% tolerance
            if (std::abs(dist - expectedRadius) > expectedRadius * 0.05f) {
                allAtVertices = false;
                std::cout << "Atom " << i << " dist from center: " << dist 
                          << " (expected ~" << expectedRadius << ")" << std::endl;
            }
        }
        
        if (allAtVertices) {
            PASS
        } else {
            FAIL("Atoms not at correct hexagon vertex positions")
        }
    }
    
    // --------------------------------------------
    // TEST 6: Verify Z-axis flattened to 0
    // --------------------------------------------
    TEST(Z_Axis_Flattened) {
        bool allFlat = true;
        for (int i = 0; i < 6; i++) {
            if (std::abs(transforms[i].z) > 0.1f) {
                allFlat = false;
            }
        }
        if (allFlat) {
            PASS
        } else {
            FAIL("Z-axis not flattened to 0")
        }
    }
    
    // --------------------------------------------
    // SUMMARY
    // --------------------------------------------
    std::cout << std::endl << "=== RESULTS ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << (testsRun - testsPassed) << std::endl;
    
    return (testsPassed == testsRun) ? 0 : 1;
}
