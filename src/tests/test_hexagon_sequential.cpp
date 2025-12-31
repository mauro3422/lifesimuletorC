/**
 * Test: Hexagon Tractor Beam Sequential Grab
 * 
 * This test simulates the REAL bug scenario:
 * 1. Create hexagon
 * 2. Grab atom A → verify isolated
 * 3. Release atom A (simulate grace period)
 * 4. Run AutonomousBonding frames → verify NO rebonding
 * 5. Grab atom B → verify it grabs ONLY atom B (not whole structure)
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <iomanip>

#include "../ecs/components.hpp"
#include "../physics/BondingSystem.hpp"
#include "../physics/AutonomousBonding.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../physics/RingChemistry.hpp"
#include "../core/MathUtils.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"

// Test data
std::vector<TransformComponent> transforms;
std::vector<AtomComponent> atoms;
std::vector<StateComponent> states;

void printAtomState(int id) {
    auto& s = states[id];
    std::cout << "  Atom " << id << ": "
              << "parent=" << std::setw(2) << s.parentEntityId
              << " cycle=" << std::setw(2) << s.cycleBondId
              << " molId=" << std::setw(2) << s.moleculeId
              << " cluster=" << (s.isClustered ? "Y" : "N")
              << " ring=" << (s.isInRing ? "Y" : "N")
              << " shield=" << (s.isShielded ? "Y" : "N")
              << " timer=" << std::fixed << std::setprecision(1) << s.releaseTimer
              << std::endl;
}

void printAllStates() {
    for (int i = 1; i <= 6; i++) printAtomState(i);
}

void createHexagon() {
    std::cout << "\n=== Creating hexagon (atoms 1-6) ===" << std::endl;
    
    transforms.clear();
    atoms.clear();
    states.clear();
    
    // Player (index 0)
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back(StateComponent{});
    
    // 6 carbons in hexagon positions
    float centerX = 100, centerY = 100;
    float radius = 42.0f;
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        float x = centerX + std::cos(angle) * radius;
        float y = centerY + std::sin(angle) * radius;
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        StateComponent st{};
        st.releaseTimer = 10.0f; // Already past grace period
        states.push_back(st);
    }
    
    // Parent-child chain
    for (int i = 2; i <= 6; i++) {
        states[i].parentEntityId = i - 1;
        states[i].isClustered = true;
        states[i].moleculeId = 1;
        states[i-1].childCount++;
        states[i-1].childList.push_back(i);
    }
    
    states[1].isClustered = true;
    states[1].moleculeId = 1;
    
    // Close ring
    states[6].cycleBondId = 1;
    states[1].cycleBondId = 6;
    
    // Ring flags
    static int ringCounter = 100;
    ringCounter++;
    for (int i = 1; i <= 6; i++) {
        states[i].isInRing = true;
        states[i].ringInstanceId = ringCounter;
        states[i].ringSize = 6;
        states[i].ringIndex = i - 1;
        states[i].dockingProgress = 1.0f;
    }
    
    printAllStates();
}

bool grabAtom(int atomId) {
    std::cout << "\n>>> GRABBING atom " << atomId << std::endl;
    
    int idx = atomId;
    
    // Check if has bonds
    bool hasBonds = states[idx].parentEntityId != -1 ||
                    states[idx].cycleBondId != -1 ||
                    states[idx].isInRing ||
                    states[idx].isClustered ||
                    BondingSystem::findLastChild(idx, states) != -1;
    
    if (hasBonds) {
        std::vector<int> oldMembers = MathUtils::getMoleculeMembers(idx, states);
        BondingSystem::breakAllBonds(idx, states, atoms);
        
        for (int oldId : oldMembers) {
            if (oldId != idx && states[oldId].isClustered) {
                BondingSystem::propagateMoleculeId(oldId, states);
            }
        }
    }
    
    // Shield (tractor active)
    states[idx].isShielded = true;
    states[idx].releaseTimer = 0.0f;
    
    // Verify isolated
    bool isolated = (states[atomId].moleculeId == atomId) &&
                    (states[atomId].parentEntityId == -1) &&
                    (states[atomId].cycleBondId == -1);
    
    std::cout << "  Isolated: " << (isolated ? "YES" : "NO") << std::endl;
    return isolated;
}

void releaseAtom(int atomId) {
    std::cout << "\n>>> RELEASING atom " << atomId << std::endl;
    states[atomId].isShielded = false;
    states[atomId].releaseTimer = 0.0f; // Just released
}

int simulateFrames(int numFrames, float dt = 0.016f) {
    std::cout << "\n>>> Simulating " << numFrames << " frames of AutonomousBonding..." << std::endl;
    
    SpatialGrid grid(50.0f);
    
    int rebondCount = 0;
    for (int frame = 0; frame < numFrames; frame++) {
        // Update grid with current transforms
        grid.update(transforms);
        
        // Increment release timers
        for (int i = 1; i < (int)states.size(); i++) {
            if (!states[i].isShielded) {
                states[i].releaseTimer += dt;
            }
        }
        
        // Reset justBonded
        for (auto& s : states) s.justBonded = false;
        
        // Run autonomous bonding
        AutonomousBonding::updateSpontaneousBonding(states, atoms, transforms, grid, nullptr, -1);
        
        // Check for rebonding
        for (int i = 1; i < (int)states.size(); i++) {
            if (states[i].justBonded) {
                rebondCount++;
                std::cout << "  [Frame " << frame << "] Atom " << i << " REBONDED!" << std::endl;
            }
        }
    }
    
    return rebondCount;
}

bool testSequentialGrab() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  TEST: Sequential Grab from Hexagon" << std::endl;
    std::cout << "========================================" << std::endl;
    
    createHexagon();
    
    // Step 1: Grab atom 1
    bool grab1 = grabAtom(1);
    if (!grab1) {
        std::cout << "FAIL: Atom 1 not isolated after grab" << std::endl;
        return false;
    }
    
    std::cout << "\nState after grabbing atom 1:" << std::endl;
    printAllStates();
    
    // Step 2: Release atom 1
    releaseAtom(1);
    
    // Step 3: Simulate 60 frames (~1 second) during grace period
    int rebonds = simulateFrames(60);
    
    std::cout << "\nState after grace period simulation:" << std::endl;
    printAllStates();
    
    if (rebonds > 0) {
        std::cout << "FAIL: " << rebonds << " rebonding events during grace period!" << std::endl;
        return false;
    }
    
    // Step 4: Verify atom 1 is STILL isolated
    bool stillIsolated = (states[1].moleculeId == 1) &&
                         (states[1].parentEntityId == -1) &&
                         (states[1].cycleBondId == -1) &&
                         (!states[1].isClustered);
    
    if (!stillIsolated) {
        std::cout << "FAIL: Atom 1 rebonded after release!" << std::endl;
        return false;
    }
    
    // Step 5: Grab atom 2 from remaining structure
    int rootBefore = MathUtils::findMoleculeRoot(2, states);
    std::cout << "\nBefore grabbing atom 2: findMoleculeRoot(2) = " << rootBefore << std::endl;
    
    bool grab2 = grabAtom(2);
    if (!grab2) {
        std::cout << "FAIL: Atom 2 not isolated after grab" << std::endl;
        return false;
    }
    
    // Step 6: Verify atom 2 grabbed individually (not the whole remaining structure)
    int remainingCount = 0;
    for (int i = 3; i <= 6; i++) {
        if (states[i].isClustered && states[i].moleculeId != 2) {
            remainingCount++;
        }
    }
    
    std::cout << "\nFinal state:" << std::endl;
    printAllStates();
    
    std::cout << "\nRemaining clustered atoms (not with atom 2): " << remainingCount << std::endl;
    
    // Success if atoms 3-6 are still in their own cluster (not grabbed with atom 2)
    if (remainingCount >= 3) {
        std::cout << "PASS: Atom 2 grabbed individually!" << std::endl;
        return true;
    } else {
        std::cout << "FAIL: Entire structure moved with atom 2!" << std::endl;
        return false;
    }
}

bool testDisassembleReassemble() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  TEST: Disassemble & Reassemble Hexagon" << std::endl;
    std::cout << "========================================" << std::endl;
    
    createHexagon();
    
    // Step 1: Disassemble by grabbing each atom
    std::cout << "\n--- Phase 1: Disassembling hexagon ---" << std::endl;
    for (int i = 1; i <= 6; i++) {
        grabAtom(i);
        releaseAtom(i);
        // Set timer past grace period so they CAN rebond later
        states[i].releaseTimer = 10.0f;
    }
    
    std::cout << "\nState after disassembly:" << std::endl;
    printAllStates();
    
    // Verify all atoms are isolated
    int isolatedCount = 0;
    for (int i = 1; i <= 6; i++) {
        if (states[i].moleculeId == i && 
            states[i].parentEntityId == -1 && 
            !states[i].isClustered) {
            isolatedCount++;
        }
    }
    std::cout << "Isolated atoms: " << isolatedCount << "/6" << std::endl;
    
    if (isolatedCount != 6) {
        std::cout << "FAIL: Not all atoms isolated after disassembly!" << std::endl;
        return false;
    }
    
    // Step 2: Move all atoms to new position (like dragging to new clay zone area)
    std::cout << "\n--- Phase 2: Moving atoms to new position ---" << std::endl;
    float newCenterX = 500.0f;
    float newCenterY = 500.0f;
    float radius = 42.0f;
    
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        float x = newCenterX + std::cos(angle) * radius;
        float y = newCenterY + std::sin(angle) * radius;
        transforms[i + 1].x = x;
        transforms[i + 1].y = y;
        // Reset timers past grace period
        states[i + 1].releaseTimer = 10.0f;
    }
    
    std::cout << "Atoms moved to center (" << newCenterX << ", " << newCenterY << ")" << std::endl;
    
    // Step 3: Simulate many frames to allow reassembly
    std::cout << "\n--- Phase 3: Simulating autonomous bonding ---" << std::endl;
    int rebonds = simulateFrames(300); // ~5 seconds of simulation
    
    std::cout << "\nState after reassembly simulation:" << std::endl;
    printAllStates();
    
    // Step 4: Verify hexagon reformed
    std::cout << "\n--- Phase 4: Verifying reassembly ---" << std::endl;
    
    // Check if atoms are clustered together
    int clusteredCount = 0;
    int sameMolecule = 0;
    int firstMolId = states[1].moleculeId;
    
    for (int i = 1; i <= 6; i++) {
        if (states[i].isClustered) clusteredCount++;
        if (states[i].moleculeId == firstMolId) sameMolecule++;
    }
    
    std::cout << "Clustered atoms: " << clusteredCount << "/6" << std::endl;
    std::cout << "Same molecule: " << sameMolecule << "/6" << std::endl;
    std::cout << "Total rebond events: " << rebonds << std::endl;
    
    // Success if most atoms are clustered together
    if (clusteredCount >= 5 && sameMolecule >= 5) {
        std::cout << "PASS: Hexagon reassembled at new position!" << std::endl;
        return true;
    } else {
        std::cout << "FAIL: Hexagon did not reassemble properly!" << std::endl;
        return false;
    }
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  HEXAGON TRACTOR SEQUENTIAL TEST" << std::endl;
    std::cout << "======================================" << std::endl;
    
    ChemistryDatabase::getInstance().reload();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    int passed = 0;
    int total = 2;
    
    // Test 1: Sequential grab (grace period blocking)
    if (testSequentialGrab()) passed++;
    
    // Test 2: Disassemble and reassemble
    if (testDisassembleReassemble()) passed++;
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "  RESULTS: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return (passed == total) ? 0 : 1;
}

