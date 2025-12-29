/**
 * test_hexagon_formation.cpp
 * 
 * PRECISE diagnostic test for C6 hexagon formation.
 * Tests exact hierarchy distances and ring closure conditions.
 */

#include <iostream>
#include <vector>
#include <cmath>

#include "raylib.h"

#include "../ecs/components.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../physics/RingChemistry.hpp"

#define TEST(name) std::cout << "[TEST] " << #name << "... "; testsRun++;
#define PASS std::cout << "PASS" << std::endl; testsPassed++;
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl;

int testsRun = 0;
int testsPassed = 0;

/**
 * Create a LINEAR chain of 6 carbons: 0 -> 1 -> 2 -> 3 -> 4 -> 5
 * This is what we need for a hexagon: cycle bond between 0 and 5
 */
void setupLinearChain6(std::vector<TransformComponent>& transforms,
                       std::vector<AtomComponent>& atoms,
                       std::vector<StateComponent>& states) {
    transforms.clear();
    atoms.clear();
    states.clear();
    
    for (int i = 0; i < 6; i++) {
        // Position in a line
        float x = i * 30.0f;
        float y = 0.0f;
        transforms.push_back({x, y, 0.0f, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});  // Carbon
        
        StateComponent state = {};
        state.isClustered = (i > 0);
        state.moleculeId = 1;
        state.parentEntityId = (i > 0) ? (i - 1) : -1;  // Linear chain
        state.parentSlotIndex = 0;
        state.dockingProgress = 1.0f;
        state.childCount = (i < 5) ? 1 : 0;  // All have 1 child except last
        state.cycleBondId = -1;
        state.isInRing = false;
        state.ringSize = 0;
        state.ringInstanceId = -1;
        state.ringIndex = -1;
        states.push_back(state);
    }
}

int main() {
    std::cout << "=== HEXAGON FORMATION DIAGNOSTIC ===" << std::endl << std::endl;
    
    ChemistryDatabase::getInstance().reload();
    
    // CRITICAL: Load structure definitions (normally done in main.cpp)
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    // Verify hexagon definition is loaded
    const StructureDefinition* hexDef = StructureRegistry::getInstance().findMatch(6, 6);
    if (hexDef) {
        std::cout << "[OK] Found hexagon structure: " << hexDef->name << " (atomCount=" << hexDef->atomCount << ")" << std::endl;
    } else {
        std::cout << "[ERROR] Hexagon structure NOT FOUND in registry!" << std::endl;
    }
    std::cout << std::endl;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // --------------------------------------------
    // TEST 1: Verify hierarchy distance calculation
    // --------------------------------------------
    TEST(Hierarchy_Distance_Linear_Chain) {
        setupLinearChain6(transforms, atoms, states);
        
        // For linear chain 0->1->2->3->4->5:
        // Distance from 0 to 5 should be 5 (path is 0-1-2-3-4-5)
        int hops05 = MathUtils::getHierarchyDistance(0, 5, states);
        int hops45 = MathUtils::getHierarchyDistance(4, 5, states);
        int hops04 = MathUtils::getHierarchyDistance(0, 4, states);
        
        std::cout << std::endl;
        std::cout << "  hops(0->5) = " << hops05 << " (expected 5)" << std::endl;
        std::cout << "  hops(4->5) = " << hops45 << " (expected 1)" << std::endl;
        std::cout << "  hops(0->4) = " << hops04 << " (expected 4)" << std::endl;
        
        if (hops05 == 5 && hops45 == 1 && hops04 == 4) {
            PASS
        } else {
            FAIL("Hierarchy distance calculation incorrect")
        }
    }
    
    // --------------------------------------------
    // TEST 2: Verify ring closure between terminals (0 and 5)
    // --------------------------------------------
    TEST(Hexagon_Cycle_Bond_0_5) {
        setupLinearChain6(transforms, atoms, states);
        
        // Try to form cycle bond between atom 0 and atom 5
        // This should create a 6-ring
        BondError result = RingChemistry::tryCycleBond(0, 5, states, atoms, transforms);
        
        std::cout << std::endl;
        std::cout << "  cycleBond(0, 5) result: " << (int)result << std::endl;
        
        if (result == BondError::SUCCESS) {
            PASS
        } else {
            std::cout << "  Error code: " << (int)result << std::endl;
            FAIL("Failed to create hexagon cycle bond 0-5")
        }
    }
    
    // --------------------------------------------
    // TEST 3: Verify all 6 atoms are in ring
    // --------------------------------------------
    TEST(All_6_Atoms_In_Ring) {
        bool allInRing = true;
        for (int i = 0; i < 6; i++) {
            if (!states[i].isInRing) {
                std::cout << std::endl << "  Atom " << i << " NOT in ring!" << std::endl;
                allInRing = false;
            }
        }
        if (allInRing) {
            PASS
        } else {
            FAIL("Not all atoms marked as isInRing")
        }
    }
    
    // --------------------------------------------
    // TEST 4: Verify ring size is 6
    // --------------------------------------------
    TEST(Ring_Size_Is_6) {
        bool sizeCorrect = true;
        for (int i = 0; i < 6; i++) {
            if (states[i].ringSize != 6) {
                std::cout << std::endl << "  Atom " << i << " ringSize: " << states[i].ringSize << std::endl;
                sizeCorrect = false;
            }
        }
        if (sizeCorrect) {
            PASS
        } else {
            FAIL("Ring size not 6")
        }
    }
    
    // --------------------------------------------
    // TEST 5: Verify ring positions (hexagon geometry)
    // --------------------------------------------
    TEST(Hexagon_Geometry) {
        // Calculate centroid
        float cx = 0, cy = 0;
        for (int i = 0; i < 6; i++) {
            cx += transforms[i].x;
            cy += transforms[i].y;
        }
        cx /= 6.0f;
        cy /= 6.0f;
        
        // Expected radius for regular hexagon: R = side / (2 * sin(π/6)) = BOND_IDEAL_DIST
        float expectedRadius = Config::BOND_IDEAL_DIST / (2.0f * std::sin(3.14159f / 6.0f));
        
        std::cout << std::endl;
        std::cout << "  Centroid: (" << cx << ", " << cy << ")" << std::endl;
        std::cout << "  Expected radius: " << expectedRadius << std::endl;
        
        bool geometryCorrect = true;
        for (int i = 0; i < 6; i++) {
            float dx = transforms[i].x - cx;
            float dy = transforms[i].y - cy;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            float error = std::abs(dist - expectedRadius);
            if (error > 2.0f) {  // Allow 2 units tolerance
                std::cout << "  Atom " << i << " dist=" << dist << " (error=" << error << ")" << std::endl;
                geometryCorrect = false;
            }
        }
        
        if (geometryCorrect) {
            PASS
        } else {
            FAIL("Hexagon geometry incorrect")
        }
    }
    
    // --------------------------------------------
    // TEST 6: Verify ring invalidation when bond breaks
    // --------------------------------------------
    TEST(Ring_Invalidation) {
        // Simulate breaking the ring by clearing isInRing and cycleBondId
        int originalRingId = states[0].ringInstanceId;
        
        // Invalidate the ring
        RingChemistry::invalidateRing(originalRingId, states);
        
        // Check all atoms are no longer in ring
        bool allCleared = true;
        for (int i = 0; i < 6; i++) {
            if (states[i].isInRing || states[i].ringInstanceId != -1 || states[i].cycleBondId != -1) {
                std::cout << std::endl;
                std::cout << "  Atom " << i << ": isInRing=" << states[i].isInRing 
                          << " ringId=" << states[i].ringInstanceId 
                          << " cycleBondId=" << states[i].cycleBondId << std::endl;
                allCleared = false;
            }
        }
        
        if (allCleared) {
            PASS
        } else {
            FAIL("Ring invalidation did not clear all flags")
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
