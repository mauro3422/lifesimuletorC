/**
 * TEST SUITE: BondingCore
 * 
 * Unit tests for atomic bonding operations.
 * Tests slot management, valency checks, and bond creation/destruction.
 * 
 * Compile: Added to build_tests.ps1
 * Run: ./test_bonding_core.exe
 */

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "raylib.h"
#include "../physics/BondingCore.hpp"
#include "../physics/BondingSystem.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../ecs/components.hpp"

// Mock InputHandler to satisfy linker dependency
namespace InputHandler {
    void setMouseCaptured(bool captured) {}
}

// Test Framework
int passed = 0;
int failed = 0;

void TEST(bool condition, const std::string& name) {
    if (condition) {
        std::cout << "[PASS] " << name << std::endl;
        passed++;
    } else {
        std::cerr << "[FAIL] " << name << std::endl;
        failed++;
    }
}

// Helper: Setup minimal test world
void setupTestWorld(std::vector<StateComponent>& states, 
                    std::vector<AtomComponent>& atoms, 
                    std::vector<TransformComponent>& transforms, 
                    int count, int atomicNumber = 6) {
    states.clear();
    atoms.clear();
    transforms.clear();
    
    states.resize(count);
    atoms.resize(count);
    transforms.resize(count);
    
    for (int i = 0; i < count; ++i) {
        states[i] = StateComponent{false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false};
        atoms[i].atomicNumber = atomicNumber;
        atoms[i].partialCharge = 0.0f;
        transforms[i] = TransformComponent{0, 0, 0, 0, 0, 0, 0};
    }
}

// ============================================================================
// TEST: canAcceptBond
// ============================================================================

void test_canAcceptBond_EmptyCarbon() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 1, 6); // Carbon
    
    const Element& carbon = ChemistryDatabase::getInstance().getElement(6);
    bool result = BondingCore::canAcceptBond(0, states, carbon);
    
    TEST(result == true, "canAcceptBond: Empty Carbon (0/4 bonds) can accept");
}

void test_canAcceptBond_FullCarbon() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 5, 6); // 1 Carbon + 4 children
    
    // Setup Carbon at 0 with 4 children
    states[0].childCount = 4;
    states[0].occupiedSlots = 0b1111; // All 4 slots occupied
    
    const Element& carbon = ChemistryDatabase::getInstance().getElement(6);
    bool result = BondingCore::canAcceptBond(0, states, carbon);
    
    TEST(result == false, "canAcceptBond: Full Carbon (4/4 bonds) cannot accept");
}

void test_canAcceptBond_Hydrogen() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 2, 1); // Hydrogen
    
    // Hydrogen already bonded to parent
    states[0].isClustered = true;
    states[0].parentEntityId = 1;
    
    const Element& hydrogen = ChemistryDatabase::getInstance().getElement(1);
    bool result = BondingCore::canAcceptBond(0, states, hydrogen);
    
    TEST(result == false, "canAcceptBond: Hydrogen with parent (1/1 bonds) cannot accept");
}

// ============================================================================
// TEST: getFirstFreeSlot
// ============================================================================

void test_getFirstFreeSlot_Empty() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 1, 6); // Carbon
    
    int slot = BondingCore::getFirstFreeSlot(0, states, atoms);
    
    TEST(slot == 0, "getFirstFreeSlot: Empty Carbon returns slot 0");
}

void test_getFirstFreeSlot_PartiallyFilled() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 3, 6); // Carbon
    
    // Slots 0 and 1 are occupied
    states[0].occupiedSlots = 0b0011;
    states[0].childCount = 2;
    
    int slot = BondingCore::getFirstFreeSlot(0, states, atoms);
    
    TEST(slot == 2, "getFirstFreeSlot: Slots 0,1 occupied returns slot 2");
}

void test_getFirstFreeSlot_Full() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 5, 6); // Carbon + 4 children
    
    // All slots occupied
    states[0].occupiedSlots = 0b1111;
    states[0].childCount = 4;
    
    int slot = BondingCore::getFirstFreeSlot(0, states, atoms);
    
    TEST(slot == -1, "getFirstFreeSlot: Full Carbon returns -1");
}

// ============================================================================
// TEST: tryBond
// ============================================================================

void test_tryBond_Success() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 2, 6); // 2 Carbons
    
    // Position them close together
    transforms[0] = {0, 0, 0, 0, 0, 0, 0};
    transforms[1] = {30, 0, 0, 0, 0, 0, 0};
    
    BondingCore::BondError result = BondingCore::tryBond(1, 0, states, atoms, transforms, true);
    
    TEST(result == BondingCore::SUCCESS, "tryBond: Two free Carbons bond successfully");
    TEST(states[1].isClustered == true, "tryBond: Source atom is now clustered");
    TEST(states[1].parentEntityId == 0, "tryBond: Source parent is target");
}

void test_tryBond_AlreadyClustered() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 3, 6);
    
    // Atom 1 already bonded to atom 2
    states[1].isClustered = true;
    states[1].parentEntityId = 2;
    states[1].dockingProgress = 1.0f; // Locked
    
    transforms[0] = {0, 0, 0, 0, 0, 0, 0};
    transforms[1] = {30, 0, 0, 0, 0, 0, 0};
    
    BondingCore::BondError result = BondingCore::tryBond(1, 0, states, atoms, transforms, false);
    
    TEST(result == BondingCore::ALREADY_CLUSTERED, "tryBond: Locked atom cannot be re-bonded");
}

// ============================================================================
// TEST: breakBond
// ============================================================================

void test_breakBond_Cleanup() {
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    std::vector<TransformComponent> transforms;
    setupTestWorld(states, atoms, transforms, 2, 6);
    
    // Setup bond: 1 -> 0
    states[1].isClustered = true;
    states[1].parentEntityId = 0;
    states[1].parentSlotIndex = 2;
    states[1].moleculeId = 0;
    
    states[0].childCount = 1;
    states[0].occupiedSlots = 0b0100; // Slot 2
    
    BondingCore::breakBond(1, states, atoms);
    
    TEST(states[1].isClustered == false, "breakBond: Child is no longer clustered");
    TEST(states[1].parentEntityId == -1, "breakBond: Child has no parent");
    TEST(states[0].childCount == 0, "breakBond: Parent childCount decremented");
    TEST((states[0].occupiedSlots & 0b0100) == 0, "breakBond: Parent slot freed");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    SetTraceLogLevel(LOG_WARNING); // Reduce noise
    
    std::cout << "=== BondingCore Unit Tests ===" << std::endl << std::endl;
    
    // Initialize chemistry database
    ChemistryDatabase::getInstance();
    
    // Run tests
    test_canAcceptBond_EmptyCarbon();
    test_canAcceptBond_FullCarbon();
    test_canAcceptBond_Hydrogen();
    
    test_getFirstFreeSlot_Empty();
    test_getFirstFreeSlot_PartiallyFilled();
    test_getFirstFreeSlot_Full();
    
    test_tryBond_Success();
    test_tryBond_AlreadyClustered();
    
    test_breakBond_Cleanup();
    
    // Summary
    std::cout << std::endl;
    std::cout << "=== RESULTS ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return (failed > 0) ? 1 : 0;
}
