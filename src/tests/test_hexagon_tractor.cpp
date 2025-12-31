/**
 * Test: Hexagon Tractor Beam Isolation
 * 
 * This test simulates grabbing atoms from a hexagon one by one
 * and verifying they become truly isolated.
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <iomanip>

#include "../ecs/components.hpp"
#include "../physics/BondingSystem.hpp"
#include "../physics/RingChemistry.hpp"
#include "../core/MathUtils.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"

// Minimal test data
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
              << " locked=" << (s.isLocked() ? "Y" : "N")
              << std::endl;
}

void printAllStates() {
    for (int i = 1; i <= 6; i++) printAtomState(i);
}

void createHexagon() {
    std::cout << "\n=== Creating hexagon (atoms 1-6) ===" << std::endl;
    
    // Clear
    transforms.clear();
    atoms.clear();
    states.clear();
    
    // Player (index 0)
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back(StateComponent{});
    
    // 6 carbons in hexagon positions
    float centerX = 0, centerY = 0;
    float radius = 42.0f; // BOND_IDEAL_DIST
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        float x = centerX + std::cos(angle) * radius;
        float y = centerY + std::sin(angle) * radius;
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back(StateComponent{});
    }
    
    // Manually create hexagon bonds: 1->2->3->4->5->6->1
    // Parent-child chain: 1 is root, 2 is child of 1, etc.
    for (int i = 2; i <= 6; i++) {
        states[i].parentEntityId = i - 1;
        states[i].isClustered = true;
        states[i].moleculeId = 1;
        states[i-1].childCount++;
        states[i-1].childList.push_back(i);
    }
    
    // Root atom setup
    states[1].isClustered = true;
    states[1].moleculeId = 1;
    
    // Close the ring with cycleBond: 6 <-> 1
    states[6].cycleBondId = 1;
    states[1].cycleBondId = 6;
    
    // Set ring flags for all
    static int ringCounter = 100;
    ringCounter++;
    for (int i = 1; i <= 6; i++) {
        states[i].isInRing = true;
        states[i].ringInstanceId = ringCounter;
        states[i].ringSize = 6;
        states[i].ringIndex = i - 1;
        states[i].dockingProgress = 1.0f;
    }
    
    std::cout << "Initial state:" << std::endl;
    printAllStates();
}

bool testGrabAtom(int atomId) {
    std::cout << "\n=== Grabbing atom " << atomId << " ===" << std::endl;
    
    // Simulate what Player::applyPhysics does
    int idx = atomId;
    int targetIdx = MathUtils::findMoleculeRoot(idx, states);
    
    std::cout << "  idx=" << idx << ", targetIdx (before break)=" << targetIdx << std::endl;
    
    // Check if has bonds (same logic as Player.cpp)
    bool hasBonds = states[idx].parentEntityId != -1 ||
                    states[idx].cycleBondId != -1 ||
                    states[idx].isInRing ||
                    states[idx].isClustered ||
                    BondingSystem::findLastChild(idx, states) != -1;
    
    std::cout << "  hasBonds=" << (hasBonds ? "YES" : "NO") << std::endl;
    
    if (hasBonds) {
        // Get old members before breaking
        std::vector<int> oldMembers = MathUtils::getMoleculeMembers(idx, states);
        std::cout << "  oldMembers: [";
        for (int m : oldMembers) std::cout << m << ",";
        std::cout << "]" << std::endl;
        
        // Break all bonds
        BondingSystem::breakAllBonds(idx, states, atoms);
        targetIdx = idx;
        
        // Re-propagate for remaining members
        for (int oldId : oldMembers) {
            if (oldId != idx && states[oldId].isClustered) {
                BondingSystem::propagateMoleculeId(oldId, states);
            }
        }
    }
    
    std::cout << "  targetIdx (after break)=" << targetIdx << std::endl;
    
    // Shield the target (simulating tractor)
    states[targetIdx].isShielded = true;
    
    std::cout << "\nState after grabbing:" << std::endl;
    printAllStates();
    
    // Verify the grabbed atom is truly isolated
    bool isolated = (states[atomId].moleculeId == atomId) &&
                    (states[atomId].parentEntityId == -1) &&
                    (states[atomId].cycleBondId == -1) &&
                    (!states[atomId].isClustered || !states[atomId].isLocked());
    
    // Unshield
    states[targetIdx].isShielded = false;
    
    std::cout << "  ISOLATION CHECK: " << (isolated ? "PASS" : "FAIL") << std::endl;
    
    return isolated;
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  HEXAGON TRACTOR ISOLATION TEST" << std::endl;
    std::cout << "======================================" << std::endl;
    
    ChemistryDatabase::getInstance().reload();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    int passed = 0;
    int total = 6;
    
    // Test grabbing each atom one by one from a fresh hexagon
    for (int testAtom = 1; testAtom <= 6; testAtom++) {
        createHexagon();
        if (testGrabAtom(testAtom)) {
            passed++;
        } else {
            std::cout << "  FAILED on atom " << testAtom << std::endl;
        }
    }
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "  RESULTS: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return (passed == total) ? 0 : 1;
}
