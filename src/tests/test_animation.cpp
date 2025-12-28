/**
 * TEST SUITE: Animation System
 * 
 * Tests for docking animations and visual state transitions.
 * Verifies smooth lerp behavior and lock states.
 * 
 * Compile: Added to build_tests.ps1
 * Run: ./test_animation.exe
 */

#include <iostream>
#include <vector>
#include <cmath>
#include "raylib.h"
#include "../physics/BondingSystem.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../ecs/components.hpp"

// Mock InputHandler
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

// ============================================================================
// TEST: Docking Progress Increment
// ============================================================================

void test_dockingProgress_Increments() {
    std::vector<StateComponent> states(2);
    std::vector<AtomComponent> atoms(2);
    std::vector<TransformComponent> transforms(2);
    
    // Setup bonded atom with initial docking progress
    atoms[0].atomicNumber = 6;
    atoms[1].atomicNumber = 6;
    
    states[0] = StateComponent{false, 0, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false};
    states[1] = StateComponent{true, 0, 0, 0, 0.0f, false, 0, 0, -1, false, 0, -1, -1, false}; // dockingProgress = 0
    
    transforms[0] = {0, 0, 0, 0, 0, 0, 0};
    transforms[1] = {30, 0, 0, 0, 0, 0, 0};
    
    float initialProgress = states[1].dockingProgress;
    
    // Run hierarchy update (which advances docking)
    BondingSystem::updateHierarchy(transforms, states, atoms);
    
    float afterProgress = states[1].dockingProgress;
    
    TEST(afterProgress > initialProgress, "Animation: dockingProgress increments each frame");
    TEST(std::abs(afterProgress - initialProgress - Config::BOND_DOCKING_SPEED) < 0.01f, 
         "Animation: Increment matches BOND_DOCKING_SPEED");
}

// ============================================================================
// TEST: isLocked Returns True at 1.0
// ============================================================================

void test_isLocked_AtFullProgress() {
    StateComponent state = {true, 0, 0, 0, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false};
    
    TEST(state.isLocked() == true, "Animation: isLocked() returns true at dockingProgress=1.0");
    
    state.dockingProgress = 0.5f;
    TEST(state.isLocked() == false, "Animation: isLocked() returns false at dockingProgress=0.5");
    
    state.dockingProgress = 0.99f;
    TEST(state.isLocked() == true, "Animation: isLocked() returns true at dockingProgress>=0.99");
    
    state.dockingProgress = 0.98f;
    TEST(state.isLocked() == false, "Animation: isLocked() returns false at dockingProgress=0.98");
}

// ============================================================================
// TEST: Docking Progress Clamped to 1.0
// ============================================================================

void test_dockingProgress_Clamped() {
    std::vector<StateComponent> states(2);
    std::vector<AtomComponent> atoms(2);
    std::vector<TransformComponent> transforms(2);
    
    atoms[0].atomicNumber = 6;
    atoms[1].atomicNumber = 6;
    
    states[0] = StateComponent{false, 0, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false};
    states[1] = StateComponent{true, 0, 0, 0, 0.98f, false, 0, 0, -1, false, 0, -1, -1, false}; // Almost full
    
    transforms[0] = {0, 0, 0, 0, 0, 0, 0};
    transforms[1] = {30, 0, 0, 0, 0, 0, 0};
    
    // Run multiple frames
    for (int i = 0; i < 10; i++) {
        BondingSystem::updateHierarchy(transforms, states, atoms);
    }
    
    TEST(states[1].dockingProgress == 1.0f, "Animation: dockingProgress clamped to 1.0");
    TEST(states[1].dockingProgress <= 1.0f, "Animation: dockingProgress never exceeds 1.0");
}

// ============================================================================
// TEST: Unclustered Atoms Don't Animate
// ============================================================================

void test_unclustered_NoAnimation() {
    std::vector<StateComponent> states(2);
    std::vector<AtomComponent> atoms(2);
    std::vector<TransformComponent> transforms(2);
    
    atoms[0].atomicNumber = 6;
    atoms[1].atomicNumber = 6;
    
    states[0] = StateComponent{false, 0, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false};
    states[1] = StateComponent{false, -1, -1, -1, 0.5f, false, 0, 0, -1, false, 0, -1, -1, false}; // Not clustered
    
    transforms[0] = {0, 0, 0, 0, 0, 0, 0};
    transforms[1] = {30, 0, 0, 0, 0, 0, 0};
    
    float initialProgress = states[1].dockingProgress;
    
    BondingSystem::updateHierarchy(transforms, states, atoms);
    
    TEST(states[1].dockingProgress == initialProgress, "Animation: Unclustered atoms don't animate");
}

// ============================================================================
// TEST: Root Atoms (No Parent) Don't Animate
// ============================================================================

void test_rootAtom_NoAnimation() {
    std::vector<StateComponent> states(1);
    std::vector<AtomComponent> atoms(1);
    std::vector<TransformComponent> transforms(1);
    
    atoms[0].atomicNumber = 6;
    states[0] = StateComponent{true, 0, -1, -1, 0.5f, false, 0, 0, -1, false, 0, -1, -1, false}; // Clustered but no parent
    transforms[0] = {0, 0, 0, 0, 0, 0, 0};
    
    float initialProgress = states[0].dockingProgress;
    
    BondingSystem::updateHierarchy(transforms, states, atoms);
    
    TEST(states[0].dockingProgress == initialProgress, "Animation: Root atoms (no parent) don't animate");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    SetTraceLogLevel(LOG_WARNING);
    
    std::cout << "=== Animation System Tests ===" << std::endl << std::endl;
    
    ChemistryDatabase::getInstance();
    
    test_dockingProgress_Increments();
    test_isLocked_AtFullProgress();
    test_dockingProgress_Clamped();
    test_unclustered_NoAnimation();
    test_rootAtom_NoAnimation();
    
    std::cout << std::endl;
    std::cout << "=== RESULTS ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return (failed > 0) ? 1 : 0;
}
