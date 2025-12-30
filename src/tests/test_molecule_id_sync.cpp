#include <iostream>
#include <vector>
#include <cassert>
#include "ecs/components.hpp"
#include "physics/MolecularHierarchy.hpp"
#include "physics/BondingCore.hpp"
#include "physics/RingChemistry.hpp"
#include "chemistry/ChemistryDatabase.hpp"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"

// Mock ECS
std::vector<TransformComponent> transforms;
std::vector<AtomComponent> atoms;
std::vector<StateComponent> states;

void spawnAtom(int atomicNumber, float x, float y) {
    std::cout << " DEBUG: Spawning atom " << states.size() << " at (" << x << "," << y << ")" << std::endl;
    TransformComponent t;
    t.x = x; t.y = y; t.z = 0;
    t.vx = 0; t.vy = 0; t.vz = 0;
    transforms.push_back(t);

    AtomComponent a;
    a.atomicNumber = atomicNumber;
    atoms.push_back(a);

    StateComponent s;
    s.moleculeId = -1; // Match main app behavior (default to -1)
    states.push_back(s);
}

int main() {
    std::cout << "=== Starting Hierarchy Sync Test ===" << std::endl;

    // 1. Setup
    ChemistryDatabase::getInstance().initialize(); // Ensure DB is ready
    for(int i=0; i<4; i++) spawnAtom(6, i * 20.0f, 0); // 4 Carbons

    // 2. Linear Formation: 0 -> 1 -> 2 -> 3
    std::cout << "[STEP 1] Forming linear molecule 0-1-2-3..." << std::endl;
    BondingCore::BondError err1 = BondingCore::tryBond(1, 0, states, atoms, transforms, true);
    if (err1 != BondingCore::SUCCESS) {
        std::cout << " FAILED: tryBond(1, 0) returned error " << (int)err1 << std::endl;
        return 1;
    }
    
    BondingCore::BondError err2 = BondingCore::tryBond(2, 1, states, atoms, transforms, true);
    if (err2 != BondingCore::SUCCESS) {
        std::cout << " FAILED: tryBond(2, 1) returned error " << (int)err2 << std::endl;
        return 1;
    }

    BondingCore::BondError err3 = BondingCore::tryBond(3, 2, states, atoms, transforms, true);
    if (err3 != BondingCore::SUCCESS) {
        std::cout << " FAILED: tryBond(3, 2) returned error " << (int)err3 << std::endl;
        return 1;
    }

    assert(states[0].moleculeId == states[3].moleculeId);
    int initialRoot = states[0].moleculeId;
    std::cout << " SUCCESS: Linear cluster root is " << initialRoot << std::endl;

    // 3. Cycle Formation: 3 -> 0 (Closing the ring)
    std::cout << "[STEP 2] Closing cycle 3-0..." << std::endl;
    RingChemistry::tryCycleBond(3, 0, states, atoms, transforms);
    
    assert(states[0].moleculeId == states[3].moleculeId);
    assert(states[0].cycleBondId == 3);
    assert(states[3].cycleBondId == 0);
    std::cout << " SUCCESS: Cycle formed. Cluster ID remains consistent." << std::endl;

    // 4. Structural Break: Break 2-3 (The tree branch)
    // Phase 42 FIX: When ring is invalidated, cycleBondId is now also cleared
    // This prevents "ghost bonds" that block reformation
    std::cout << "[STEP 3] Breaking bond 2-3 (Hierarchy branch)..." << std::endl;
    BondingCore::breakBond(3, states, atoms); // Break parent-child 2->3

    std::cout << " DEBUG after break: ";
    for(int i=0; i<4; i++) std::cout << "Atom " << i << " molId=" << states[i].moleculeId << " | ";
    std::cout << std::endl;

    assert(states[3].parentEntityId == -1);
    // Phase 42: cycleBondId is now cleared when ring is invalidated
    assert(states[3].cycleBondId == -1); // Cleaned up with ring invalidation
    assert(states[3].moleculeId == 3); // Atom 3 is now isolated (root of itself)
    std::cout << " SUCCESS: Ring invalidation also cleaned cycleBondId (Phase 42 fix)." << std::endl;

    // Since atom 3 is now isolated, Step 4 (breaking cycle) is no longer needed
    std::cout << "[STEP 4] Verifying final state..." << std::endl;

    std::cout << " DEBUG final state: ";
    for(int i=0; i<4; i++) std::cout << "Atom " << i << " molId=" << states[i].moleculeId << " | ";
    std::cout << std::endl;

    assert(states[3].cycleBondId == -1);
    assert(states[3].moleculeId == 3); // 3 should be its own root now
    assert(states[0].moleculeId != 3);
    std::cout << " SUCCESS: Atom 3 isolated. moleculeId correctly set." << std::endl;

    std::cout << "=== ALL HIERARCHY TESTS PASSED ===" << std::endl;
    return 0;
}
