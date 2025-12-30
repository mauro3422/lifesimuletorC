/**
 * TEST: Tractor Beam Dynamics and Shield Cleanup
 * 
 * Simulates the tractor beam lifecycle:
 * 1. Capturing an atom
 * 2. Moving it (simulating mouse drag)
 * 3. Breaking bonds during drag
 * 4. Releasing and verifying shield cleanup
 * 5. Verifying atoms can rebond after release
 */

#include <iostream>
#include <vector>
#include <cassert>
#include "ecs/components.hpp"
#include "physics/MolecularHierarchy.hpp"
#include "physics/BondingCore.hpp"
#include "physics/AutonomousBonding.hpp"
#include "physics/SpatialGrid.hpp"
#include "chemistry/ChemistryDatabase.hpp"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"

// Mock ECS
std::vector<TransformComponent> transforms;
std::vector<AtomComponent> atoms;
std::vector<StateComponent> states;

void resetECS() {
    transforms.clear();
    atoms.clear();
    states.clear();
}

int spawnAtom(int atomicNumber, float x, float y) {
    int id = (int)states.size();
    TransformComponent t = {x, y, 0, 0, 0, 0, 0};
    transforms.push_back(t);
    atoms.push_back({atomicNumber, 0.0f});
    states.push_back(StateComponent{});
    return id;
}

void simulateShield(int atomId, bool shielded) {
    std::vector<int> members = MathUtils::getMoleculeMembers(atomId, states);
    for (int m : members) {
        states[m].isShielded = shielded;
    }
}

bool testShieldCleanup() {
    std::cout << "\n=== TEST: Shield Cleanup After Release ===" << std::endl;
    resetECS();
    
    // Create 3 bonded carbons: 0 -> 1 -> 2
    spawnAtom(6, 0, 0);
    spawnAtom(6, 30, 0);
    spawnAtom(6, 60, 0);
    
    BondingCore::tryBond(1, 0, states, atoms, transforms, true);
    BondingCore::tryBond(2, 1, states, atoms, transforms, true);
    
    std::cout << " Created molecule 0-1-2" << std::endl;
    
    // Simulate tractor beam capture on atom 1
    simulateShield(1, true);
    std::cout << " Shielded molecule (simulating tractor capture)" << std::endl;
    
    // Verify all are shielded
    bool allShielded = states[0].isShielded && states[1].isShielded && states[2].isShielded;
    if (!allShielded) {
        std::cout << " FAIL: Not all atoms shielded. 0=" << states[0].isShielded 
                  << " 1=" << states[1].isShielded << " 2=" << states[2].isShielded << std::endl;
        return false;
    }
    std::cout << " All atoms shielded correctly." << std::endl;
    
    // Simulate release
    simulateShield(1, false);
    std::cout << " Released (shields cleared)" << std::endl;
    
    // Verify all shields cleared
    bool allCleared = !states[0].isShielded && !states[1].isShielded && !states[2].isShielded;
    if (!allCleared) {
        std::cout << " FAIL: Shields not cleared! 0=" << states[0].isShielded 
                  << " 1=" << states[1].isShielded << " 2=" << states[2].isShielded << std::endl;
        return false;
    }
    std::cout << " SUCCESS: All shields cleared." << std::endl;
    return true;
}

bool testRebondingAfterRelease() {
    std::cout << "\n=== TEST: Rebonding After Tractor Release ===" << std::endl;
    resetECS();
    
    // Create 2 separate carbons
    int a = spawnAtom(6, 0, 0);
    int b = spawnAtom(6, 40, 0); // Within BOND_AUTO_RANGE (65)
    
    std::cout << " Two isolated carbons at (0,0) and (40,0)" << std::endl;
    
    // Simulate: Shield atom A, then release
    states[a].isShielded = true;
    states[a].isShielded = false;
    states[a].releaseTimer = 0.5f; // Recently released
    
    // Verify isLocked returns false when shielded
    states[a].isShielded = true;
    states[a].isClustered = true;
    states[a].dockingProgress = 1.0f;
    
    bool lockedWhileShielded = states[a].isLocked();
    if (lockedWhileShielded) {
        std::cout << " FAIL: isLocked() returned true while shielded!" << std::endl;
        return false;
    }
    std::cout << " isLocked() correctly returns false when shielded." << std::endl;
    
    // Reset for bonding test
    states[a].isShielded = false;
    states[a].isClustered = false;
    states[a].dockingProgress = 1.0f;
    
    // Try to bond them (simulating spontaneous bonding)
    BondingCore::BondError err = BondingCore::tryBond(a, b, states, atoms, transforms, true);
    if (err != BondingCore::SUCCESS) {
        std::cout << " FAIL: tryBond returned error " << (int)err << std::endl;
        return false;
    }
    
    std::cout << " SUCCESS: Atoms rebonded after release." << std::endl;
    return true;
}

bool testMoleculeRootTracking() {
    std::cout << "\n=== TEST: Molecule Root Tracking During Break ===" << std::endl;
    resetECS();
    
    // Create molecule: 0 -> 1 -> 2
    spawnAtom(6, 0, 0);
    spawnAtom(6, 30, 0);
    spawnAtom(6, 60, 0);
    
    BondingCore::tryBond(1, 0, states, atoms, transforms, true);
    BondingCore::tryBond(2, 1, states, atoms, transforms, true);
    
    int rootBefore = MathUtils::findMoleculeRoot(2, states);
    std::cout << " Root of atom 2 before break: " << rootBefore << std::endl;
    
    // Break bond 1-2
    BondingCore::breakBond(2, states, atoms);
    
    int rootAfter = MathUtils::findMoleculeRoot(2, states);
    std::cout << " Root of atom 2 after break: " << rootAfter << std::endl;
    
    if (rootAfter == rootBefore) {
        std::cout << " FAIL: Root did not change after break!" << std::endl;
        return false;
    }
    
    // Atom 2 should now be its own root
    if (rootAfter != 2) {
        std::cout << " FAIL: Atom 2 should be its own root, but got " << rootAfter << std::endl;
        return false;
    }
    
    std::cout << " SUCCESS: Root tracking works correctly." << std::endl;
    return true;
}

bool testProximityBonding() {
    std::cout << "\n=== TEST: Proximity Bonding (Simulated Tractor Grouping) ===" << std::endl;
    resetECS();
    
    // Create 4 carbons in a square (within bonding range of each other)
    float spacing = 50.0f; // < BOND_AUTO_RANGE (65)
    spawnAtom(6, 0, 0);
    spawnAtom(6, spacing, 0);
    spawnAtom(6, spacing, spacing);
    spawnAtom(6, 0, spacing);
    
    std::cout << " Created 4 isolated carbons in a square with spacing " << spacing << std::endl;
    
    // Simulate spontaneous bonding by manually calling tryBond for nearby atoms
    int bondCount = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            float dx = transforms[i].x - transforms[j].x;
            float dy = transforms[i].y - transforms[j].y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist < Config::BOND_AUTO_RANGE) {
                // Check if bond is possible
                int rootI = MathUtils::findMoleculeRoot(i, states);
                int rootJ = MathUtils::findMoleculeRoot(j, states);
                
                if (rootI != rootJ) {
                    BondingCore::BondError err = BondingCore::tryBond(i, j, states, atoms, transforms, true);
                    if (err == BondingCore::SUCCESS) {
                        bondCount++;
                        std::cout << "  Bonded " << i << " to " << j << " (dist=" << dist << ")" << std::endl;
                    }
                }
            }
        }
    }
    
    std::cout << " Formed " << bondCount << " bonds." << std::endl;
    
    // Should have at least 3 bonds to connect all 4 atoms (tree structure)
    if (bondCount < 3) {
        std::cout << " FAIL: Expected at least 3 bonds, got " << bondCount << std::endl;
        return false;
    }
    
    // Verify all atoms share the same moleculeId
    int root0 = states[0].moleculeId;
    bool allSameRoot = true;
    for (int i = 1; i < 4; i++) {
        if (states[i].moleculeId != root0) {
            std::cout << " FAIL: Atom " << i << " has different root " << states[i].moleculeId 
                      << " vs " << root0 << std::endl;
            allSameRoot = false;
        }
    }
    
    if (!allSameRoot) return false;
    
    std::cout << " SUCCESS: All atoms share moleculeId " << root0 << std::endl;
    return true;
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  TRACTOR BEAM DYNAMICS TEST SUITE" << std::endl;
    std::cout << "======================================" << std::endl;
    
    ChemistryDatabase::getInstance().initialize();
    
    int passed = 0;
    int total = 4;
    
    if (testShieldCleanup()) passed++;
    if (testRebondingAfterRelease()) passed++;
    if (testMoleculeRootTracking()) passed++;
    if (testProximityBonding()) passed++;
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "  RESULTS: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return (passed == total) ? 0 : 1;
}
