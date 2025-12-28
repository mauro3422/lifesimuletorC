/**
 * test_ring_topology.cpp
 * Validates that ring formation creates correct member ordering and positions
 * to prevent crossed bond visualization.
 * 
 * Key Tests:
 * 1. Ring member order matches hierarchical adjacency
 * 2. Hard-snap positions atoms in correct square layout
 * 3. Each position-adjacent pair is bond-connected
 * 4. Triangles (3-atom) are rejected
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#include "raylib.h"  // Use real raylib

#include "../ecs/components.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"

// Include RingChemistry to test it directly
#include "../physics/RingChemistry.hpp"

#define TEST(name) std::cout << "[TEST] " << #name << "... "; testsRun++;
#define PASS std::cout << "PASS" << std::endl; testsPassed++;
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl;

int testsRun = 0;
int testsPassed = 0;

/**
 * Helper: Create a chain of 4 atoms with specific hierarchy
 * Hierarchy:  A(0) -> B(1) -> C(2), A(0) -> D(3)
 * This forms a "Y" shape where cycle bond C-D creates a 4-ring
 */
void setupHierarchyY(std::vector<TransformComponent>& transforms,
                     std::vector<AtomComponent>& atoms,
                     std::vector<StateComponent>& states) {
    transforms.clear();
    atoms.clear();
    states.clear();
    
    // Atom 0: A (root)
    transforms.push_back({0.0f, 0.0f, 0.0f, 0, 0, 0, 0});
    atoms.push_back({6, 0.0f});  // Carbon
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
    
    // Atom 1: B (child of A, slot 0)
    transforms.push_back({50.0f, 0.0f, 0.0f, 0, 0, 0, 0});
    atoms.push_back({6, 0.0f});
    states.push_back({true, 1, 0, 0, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
    states[0].isClustered = true;
    states[0].moleculeId = 1;
    states[0].childCount = 2;  // A has 2 children (B and D)
    states[0].occupiedSlots = 0b11;  // Slots 0 and 1 occupied
    
    // Atom 2: C (child of B, slot 0)
    transforms.push_back({100.0f, 0.0f, 0.0f, 0, 0, 0, 0});
    atoms.push_back({6, 0.0f});
    states.push_back({true, 1, 1, 0, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
    states[1].childCount = 1;  // B has 1 child (C)
    states[1].occupiedSlots = 0b1;
    
    // Atom 3: D (child of A, slot 1)
    transforms.push_back({0.0f, 50.0f, 0.0f, 0, 0, 0, 0});
    atoms.push_back({6, 0.0f});
    states.push_back({true, 1, 0, 1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
}

/**
 * Helper: Check if two atoms are bonded (hierarchically or via cycle)
 */
bool areBonded(int a, int b, const std::vector<StateComponent>& states) {
    // Check parent-child relationship
    if (states[a].parentEntityId == b || states[b].parentEntityId == a) {
        return true;
    }
    // Check cycle bond
    if (states[a].cycleBondId == b || states[b].cycleBondId == a) {
        return true;
    }
    return false;
}

/**
 * Helper: Calculate distance between two atoms
 */
float atomDistance(int a, int b, const std::vector<TransformComponent>& transforms) {
    float dx = transforms[a].x - transforms[b].x;
    float dy = transforms[a].y - transforms[b].y;
    return std::sqrt(dx*dx + dy*dy);
}

/**
 * Helper: Get position "index" based on quadrant (for square validation)
 * Returns 0=top-left, 1=top-right, 2=bottom-right, 3=bottom-left
 */
int getQuadrant(float x, float y, float cx, float cy) {
    if (x < cx && y < cy) return 0;  // top-left
    if (x >= cx && y < cy) return 1; // top-right
    if (x >= cx && y >= cy) return 2; // bottom-right
    return 3; // bottom-left
}

int main() {
    std::cout << "=== RING TOPOLOGY TESTS ===" << std::endl << std::endl;
    
    ChemistryDatabase::getInstance().reload();
    
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
    // TEST 2: Valid 4-ring formation
    // --------------------------------------------
    TEST(Valid_4Ring_Formation) {
        setupHierarchyY(transforms, atoms, states);
        
        // Try to form cycle bond between C(2) and D(3)
        BondError result = RingChemistry::tryCycleBond(2, 3, states, atoms, transforms);
        
        if (result == BondError::SUCCESS) {
            // Verify cycle bonds were set
            if (states[2].cycleBondId == 3 && states[3].cycleBondId == 2) {
                PASS
            } else {
                FAIL("Cycle bond IDs not set correctly")
            }
        } else {
            FAIL("Expected SUCCESS for 4-ring formation")
        }
    }
    
    // --------------------------------------------
    // TEST 3: All ring members marked as isInRing
    // --------------------------------------------
    TEST(All_Members_In_Ring) {
        // Using state from previous test
        bool allInRing = states[0].isInRing && states[1].isInRing && 
                        states[2].isInRing && states[3].isInRing;
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
        // All atoms should have ringSize = 4
        bool sizeCorrect = states[0].ringSize == 4 && states[1].ringSize == 4 &&
                          states[2].ringSize == 4 && states[3].ringSize == 4;
        if (sizeCorrect) {
            PASS
        } else {
            std::cout << "Ring sizes: " << states[0].ringSize << ", " << states[1].ringSize 
                      << ", " << states[2].ringSize << ", " << states[3].ringSize << std::endl;
            FAIL("Ring size not 4 for all atoms")
        }
    }
    
    // --------------------------------------------
    // TEST 5: Positions form a proper square
    // --------------------------------------------
    TEST(Square_Positions) {
        // Calculate centroid
        float cx = 0, cy = 0;
        for (int i = 0; i < 4; i++) {
            cx += transforms[i].x;
            cy += transforms[i].y;
        }
        cx /= 4; cy /= 4;
        
        // Each atom should be at a corner of a square around the centroid
        float expectedDist = Config::BOND_IDEAL_DIST * 0.5f * std::sqrt(2.0f);
        
        bool allAtCorners = true;
        for (int i = 0; i < 4; i++) {
            float dist = std::sqrt(std::pow(transforms[i].x - cx, 2) + 
                                   std::pow(transforms[i].y - cy, 2));
            if (std::abs(dist - expectedDist) > 1.0f) {
                allAtCorners = false;
                std::cout << "Atom " << i << " dist from center: " << dist 
                          << " (expected ~" << expectedDist << ")" << std::endl;
            }
        }
        
        if (allAtCorners) {
            PASS
        } else {
            FAIL("Atoms not at correct square corner positions")
        }
    }
    
    // --------------------------------------------
    // TEST 6: Position-adjacent atoms are bond-connected
    // This is the CRITICAL test for crossed bonds
    // --------------------------------------------
    TEST(Adjacent_Positions_Bonded) {
        // Map each atom to its quadrant
        float cx = 0, cy = 0;
        for (int i = 0; i < 4; i++) {
            cx += transforms[i].x; cy += transforms[i].y;
        }
        cx /= 4; cy /= 4;
        
        int quadrants[4];
        for (int i = 0; i < 4; i++) {
            quadrants[i] = getQuadrant(transforms[i].x, transforms[i].y, cx, cy);
        }
        
        // Find which atom is at each quadrant
        int atomAtQuadrant[4] = {-1, -1, -1, -1};
        for (int i = 0; i < 4; i++) {
            atomAtQuadrant[quadrants[i]] = i;
        }
        
        // Check that adjacent quadrants have bonded atoms
        // Quadrant adjacency: 0-1, 1-2, 2-3, 3-0
        bool allAdjacent = true;
        for (int q = 0; q < 4; q++) {
            int nextQ = (q + 1) % 4;
            int atomA = atomAtQuadrant[q];
            int atomB = atomAtQuadrant[nextQ];
            
            if (atomA == -1 || atomB == -1) {
                std::cout << "Missing atom at quadrant " << q << " or " << nextQ << std::endl;
                allAdjacent = false;
                continue;
            }
            
            if (!areBonded(atomA, atomB, states)) {
                std::cout << "Atoms " << atomA << " (q" << q << ") and " 
                          << atomB << " (q" << nextQ << ") are NOT bonded but should be!" << std::endl;
                allAdjacent = false;
            }
        }
        
        if (allAdjacent) {
            PASS
        } else {
            FAIL("Position-adjacent atoms are not bond-connected (crossed bonds!)")
        }
    }
    
    // --------------------------------------------
    // TEST 7: Verify Z-axis flattened to 0
    // --------------------------------------------
    TEST(Z_Axis_Flattened) {
        bool allFlat = true;
        for (int i = 0; i < 4; i++) {
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
