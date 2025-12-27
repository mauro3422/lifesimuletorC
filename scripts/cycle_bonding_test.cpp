/**
 * CYCLE BONDING DIAGNOSTIC TEST
 * 
 * This standalone test simulates the cycle bonding logic to identify
 * why 4-carbon chains aren't closing into rings.
 * 
 * Compile: g++ -o cycle_test scripts/cycle_bonding_test.cpp -std=c++17
 * Run: ./cycle_test
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// --- MOCK CONFIG ---
const float BOND_AUTO_RANGE = 50.0f;
const float BOND_IDEAL_DIST = 42.0f;

// --- MOCK STRUCTURES ---
struct StateComponent {
    int parentEntityId = -1;
    int childCount = 0;
    int cycleBondId = -1;
    int occupiedSlots = 0;
    bool isShielded = false;
    bool isClustered = false;
};

struct TransformComponent {
    float x, y, z;
};

struct AtomComponent {
    int atomicNumber; // 6 = Carbon
};

// --- UTILITY ---
int findMoleculeRoot(int id, const std::vector<StateComponent>& states) {
    while (states[id].parentEntityId != -1) {
        id = states[id].parentEntityId;
    }
    return id;
}

float distance(const TransformComponent& a, const TransformComponent& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

// --- MAIN TEST ---
int main() {
    std::cout << "=== CYCLE BONDING DIAGNOSTIC TEST ===" << std::endl;
    std::cout << "Simulating a C-C-C-C chain..." << std::endl << std::endl;

    // Create 4 Carbon atoms in a chain
    // C0 -> C1 -> C2 -> C3
    std::vector<StateComponent> states(4);
    std::vector<AtomComponent> atoms(4);
    std::vector<TransformComponent> transforms(4);

    // Setup chain hierarchy: 0 is root, 1->0, 2->1, 3->2
    states[0].parentEntityId = -1; // ROOT
    states[0].childCount = 1;
    
    states[1].parentEntityId = 0;
    states[1].childCount = 1;
    states[0].childCount = 1; // 0 has 1 child (1)
    
    states[2].parentEntityId = 1;
    states[2].childCount = 1;
    states[1].childCount = 1; // 1 has 1 child (2)
    
    states[3].parentEntityId = 2;
    states[3].childCount = 0; // LEAF
    states[2].childCount = 1; // 2 has 1 child (3)

    // Setup positions in a "U" shape
    transforms[0] = {0, 0, 0};
    transforms[1] = {40, 0, 0};
    transforms[2] = {40, 40, 0};
    transforms[3] = {0, 40, 0};

    // Set atoms
    for (auto& a : atoms) a.atomicNumber = 6; // Carbon

    // --- DEBUG: Print chain structure ---
    std::cout << "--- CHAIN STRUCTURE ---" << std::endl;
    for (int i = 0; i < 4; i++) {
        int bonds = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        std::cout << "Atom " << i 
                  << " | Parent: " << states[i].parentEntityId 
                  << " | Children: " << states[i].childCount
                  << " | Total Bonds: " << bonds
                  << " | IsTerminal: " << (bonds == 1 ? "YES" : "NO")
                  << std::endl;
    }

    // --- CHECK TERMINALS ---
    std::cout << std::endl << "--- TERMINAL CHECK ---" << std::endl;
    int terminalCount = 0;
    int terminals[2] = {-1, -1};
    for (int i = 0; i < 4; i++) {
        int bonds = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        if (bonds == 1) {
            if (terminalCount < 2) terminals[terminalCount] = i;
            terminalCount++;
        }
    }
    std::cout << "Found " << terminalCount << " terminals: ";
    if (terminalCount >= 2) {
        std::cout << terminals[0] << " and " << terminals[1] << std::endl;
    } else {
        std::cout << "INSUFFICIENT!" << std::endl;
    }

    // --- CHECK SAME MOLECULE ---
    std::cout << std::endl << "--- MOLECULE ROOT CHECK ---" << std::endl;
    int root0 = findMoleculeRoot(terminals[0], states);
    int root3 = findMoleculeRoot(terminals[1], states);
    std::cout << "Root of Atom " << terminals[0] << ": " << root0 << std::endl;
    std::cout << "Root of Atom " << terminals[1] << ": " << root3 << std::endl;
    std::cout << "Same Molecule: " << (root0 == root3 ? "YES" : "NO") << std::endl;

    // --- CHECK DISTANCE ---
    std::cout << std::endl << "--- DISTANCE CHECK ---" << std::endl;
    float dist = distance(transforms[terminals[0]], transforms[terminals[1]]);
    std::cout << "Distance between terminals: " << dist << " px" << std::endl;
    std::cout << "BOND_AUTO_RANGE: " << BOND_AUTO_RANGE << " px" << std::endl;
    std::cout << "In Range: " << (dist < BOND_AUTO_RANGE ? "YES" : "NO") << std::endl;

    // --- SIMULATE CYCLE CHECK LOGIC ---
    std::cout << std::endl << "--- CYCLE CHECK SIMULATION ---" << std::endl;
    int i = terminals[0];
    int j = terminals[1];
    
    bool directlyBonded = (states[i].parentEntityId == j || states[j].parentEntityId == i);
    bool alreadyCycled = (states[i].cycleBondId == j || states[j].cycleBondId == i);
    
    std::cout << "Directly Bonded: " << (directlyBonded ? "YES (skip)" : "NO (ok)") << std::endl;
    std::cout << "Already Cycled: " << (alreadyCycled ? "YES (skip)" : "NO (ok)") << std::endl;

    int bondsI = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
    int bondsJ = (states[j].parentEntityId != -1 ? 1 : 0) + states[j].childCount;
    
    std::cout << "BondsI: " << bondsI << ", BondsJ: " << bondsJ << std::endl;
    
    if (bondsI == 1 && bondsJ == 1) {
        std::cout << "[PASS] Both are terminals! Cycle should form." << std::endl;
    } else {
        std::cout << "[FAIL] Not both terminals. Cycle WON'T form." << std::endl;
    }

    // --- FINAL VERDICT ---
    std::cout << std::endl << "=== DIAGNOSTIC RESULT ===" << std::endl;
    bool canCycle = !directlyBonded && !alreadyCycled && (bondsI == 1 && bondsJ == 1) && (dist < BOND_AUTO_RANGE) && (root0 == root3);
    if (canCycle) {
        std::cout << "[SUCCESS] All conditions met. Cycle SHOULD form." << std::endl;
    } else {
        std::cout << "[FAILURE] Cycle cannot form. Check conditions above." << std::endl;
    }

    return 0;
}
