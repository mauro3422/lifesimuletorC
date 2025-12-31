/**
 * Test: Hexagon Tractor Beam - Exact Flow Simulation
 * 
 * Simulates the EXACT behavior from Player.cpp::applyPhysics
 * to identify which atoms can be grabbed and which drag the structure.
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

#include "../ecs/components.hpp"
#include "../physics/BondingSystem.hpp"
#include "../physics/AutonomousBonding.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../physics/RingChemistry.hpp"
#include "../core/MathUtils.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"

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
              << " children=" << s.childCount
              << std::endl;
}

void printAllStates() {
    for (int i = 1; i <= 6; i++) printAtomState(i);
}

void createHexagon() {
    transforms.clear();
    atoms.clear();
    states.clear();
    
    // Player (index 0)
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back(StateComponent{});
    
    // 6 carbons
    float centerX = 100, centerY = 100, radius = 42.0f;
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        float x = centerX + std::cos(angle) * radius;
        float y = centerY + std::sin(angle) * radius;
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        StateComponent st{};
        st.releaseTimer = 10.0f;
        states.push_back(st);
    }
    
    // Parent-child chain: 1->2->3->4->5->6
    for (int i = 2; i <= 6; i++) {
        states[i].parentEntityId = i - 1;
        states[i].isClustered = true;
        states[i].moleculeId = 1;
        states[i-1].childCount++;
        states[i-1].childList.push_back(i);
    }
    
    states[1].isClustered = true;
    states[1].moleculeId = 1;
    
    // Cycle: 6 <-> 1
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
}

/**
 * Simulates EXACTLY what Player::applyPhysics does when grabbing an atom.
 * Returns the ID of what WOULD be moved (the atom itself if isolated correctly,
 * or the root of the whole structure if isolation failed).
 */
int simulateTractorGrab(int clickedAtomId) {
    int idx = clickedAtomId;
    
    // This is from Player.cpp line 92
    int rootCheck = MathUtils::findMoleculeRoot(idx, states);
    
    std::cout << "  [GRAB] Clicked: " << idx << ", findMoleculeRoot() = " << rootCheck << std::endl;
    
    // Player.cpp lines 100-117: Break bonds on capture
    bool hasBonds = states[idx].parentEntityId != -1 ||
                    states[idx].cycleBondId != -1 ||
                    states[idx].isInRing ||
                    states[idx].isClustered ||
                    BondingSystem::findLastChild(idx, states) != -1;
    
    std::cout << "  [GRAB] hasBonds = " << (hasBonds ? "YES" : "NO") << std::endl;
    
    if (hasBonds) {
        std::vector<int> oldMembers = MathUtils::getMoleculeMembers(idx, states);
        std::cout << "  [GRAB] oldMembers.size() = " << oldMembers.size() << std::endl;
        
        BondingSystem::breakAllBonds(idx, states, atoms);
        
        for (int oldId : oldMembers) {
            if (oldId != idx && states[oldId].isClustered) {
                BondingSystem::propagateMoleculeId(oldId, states);
            }
        }
    }
    
    // Shield the atom
    states[idx].isShielded = true;
    states[idx].releaseTimer = 0.0f;
    
    // Return what findMoleculeRoot would return NOW (after breaking)
    int finalRoot = MathUtils::findMoleculeRoot(idx, states);
    std::cout << "  [GRAB] After break: findMoleculeRoot(" << idx << ") = " << finalRoot << std::endl;
    
    bool isolated = (finalRoot == idx) && 
                    (states[idx].parentEntityId == -1) &&
                    (states[idx].cycleBondId == -1) &&
                    (!states[idx].isClustered || states[idx].childList.empty());
    
    std::cout << "  [GRAB] Isolated = " << (isolated ? "YES" : "NO") << std::endl;
    
    return isolated ? idx : finalRoot;
}

void simulateRelease(int atomId) {
    states[atomId].isShielded = false;
    states[atomId].releaseTimer = 10.0f; // Past grace period
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  TRACTOR BEAM EXACT FLOW TEST" << std::endl;
    std::cout << "======================================" << std::endl;
    
    ChemistryDatabase::getInstance().reload();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    // Test with randomized order
    std::vector<int> atomOrder = {1, 2, 3, 4, 5, 6};
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(atomOrder.begin(), atomOrder.end(), g);
    
    std::cout << "\n--- Randomized atom order: [";
    for (int a : atomOrder) std::cout << a << ",";
    std::cout << "] ---" << std::endl;
    
    createHexagon();
    
    std::cout << "\n=== Initial State ===" << std::endl;
    printAllStates();
    
    int successCount = 0;
    
    for (int i = 0; i < 6; i++) {
        int atomId = atomOrder[i];
        
        std::cout << "\n=== Attempt " << (i+1) << ": Grabbing Atom " << atomId << " ===" << std::endl;
        
        int movedId = simulateTractorGrab(atomId);
        
        if (movedId == atomId) {
            std::cout << "  RESULT: SUCCESS - Only atom " << atomId << " moved" << std::endl;
            successCount++;
        } else {
            std::cout << "  RESULT: FAILED - Would move root " << movedId << " (entire structure)" << std::endl;
        }
        
        // Release this atom before next grab
        simulateRelease(atomId);
        
        std::cout << "\n--- State after attempt ---" << std::endl;
        printAllStates();
    }
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "  RESULTS: " << successCount << "/6 atoms could be grabbed individually" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return (successCount == 6) ? 0 : 1;
}
