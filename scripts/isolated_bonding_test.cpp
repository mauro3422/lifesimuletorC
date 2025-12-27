/**
 * ISOLATED ATOM BONDING TEST
 * 
 * Simulates user workflow:
 * 1. Capture 4 individual Carbon atoms (each becomes isolated)
 * 2. Release them near each other
 * 3. Check if they bond into a chain
 * 4. Check if chain terminals are detected for ring closure
 * 
 * Compile: g++ -o isolated_test scripts/isolated_bonding_test.cpp -std=c++17
 * Run: ./isolated_test
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// --- MOCK CONFIG ---
const float BOND_AUTO_RANGE = 50.0f;

// --- MOCK STRUCTURES ---
struct StateComponent {
    int parentEntityId = -1;
    int childCount = 0;
    int cycleBondId = -1;
    int moleculeId = -1;
    bool isClustered = false;
};

struct TransformComponent {
    float x, y, z;
};

// --- UTILITY ---
int findMoleculeRoot(int id, const std::vector<StateComponent>& states) {
    if (id < 0) return -1;
    int rootId = id;
    while (states[rootId].parentEntityId != -1) {
        rootId = states[rootId].parentEntityId;
    }
    return rootId;
}

float distance(const TransformComponent& a, const TransformComponent& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

// --- SIMULATE USER WORKFLOW ---
int main() {
    std::cout << "=== ISOLATED CARBON BONDING TEST ===" << std::endl;
    std::cout << "Simulating user dropping 4 isolated C atoms..." << std::endl << std::endl;

    // Create 4 ISOLATED Carbon atoms (as if user captured them individually)
    std::vector<StateComponent> states(4);
    std::vector<TransformComponent> transforms(4);

    // All atoms start ISOLATED (no parent, no children, not clustered)
    for (int i = 0; i < 4; i++) {
        states[i].parentEntityId = -1;
        states[i].childCount = 0;
        states[i].cycleBondId = -1;
        states[i].moleculeId = i; // Each is its own root
        states[i].isClustered = false; // KEY: Isolated atoms are NOT clustered
    }

    // Position them in a square (close enough to bond)
    transforms[0] = {0, 0, 0};   // Top-left
    transforms[1] = {40, 0, 0};  // Top-right
    transforms[2] = {40, 40, 0}; // Bottom-right
    transforms[3] = {0, 40, 0};  // Bottom-left

    // --- DEBUG: Print initial state ---
    std::cout << "--- INITIAL STATE (After user drops all 4) ---" << std::endl;
    for (int i = 0; i < 4; i++) {
        int root = findMoleculeRoot(i, states);
        int bonds = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        std::cout << "Atom " << i 
                  << " | Root: " << root
                  << " | Clustered: " << (states[i].isClustered ? "YES" : "NO")
                  << " | Bonds: " << bonds
                  << std::endl;
    }

    // --- SIMULATE MULTIPLE BONDING TICKS ---
    for (int tick = 1; tick <= 5; tick++) {
        std::cout << std::endl << "--- TICK " << tick << ": updateSpontaneousBonding ---" << std::endl;
        
        for (int i = 0; i < 4; i++) {
            // Line 286: if (states[i].isClustered) continue; 
            if (states[i].isClustered) {
                std::cout << "[SKIP] Atom " << i << " is already clustered." << std::endl;
                continue;
            }

            for (int j = 0; j < 4; j++) {
                if (j <= i) continue; // Avoid duplicate pairs
                
                float dist = distance(transforms[i], transforms[j]);
                
                if (dist < BOND_AUTO_RANGE) {
                    int rootI = findMoleculeRoot(i, states);
                    int rootJ = findMoleculeRoot(j, states);
                    
                    if (rootI != rootJ) {
                        std::cout << "[MERGE] Atom " << j << " -> " << i << " (dist: " << dist << ")" << std::endl;
                        
                        // Simulate merge: j becomes child of i
                        states[j].parentEntityId = i;
                        states[j].isClustered = true;
                        states[i].childCount++;
                        break; // One bond per atom per tick (like real code)
                    }
                }
            }
        }
        
        // Print state after tick
        std::cout << "State after tick " << tick << ":" << std::endl;
        for (int i = 0; i < 4; i++) {
            int root = findMoleculeRoot(i, states);
            std::cout << "  Atom " << i << " | Root: " << root << " | Parent: " << states[i].parentEntityId << std::endl;
        }
    }

    // --- DEBUG: Print final state ---
    std::cout << std::endl << "--- FINAL STATE (After one bonding tick) ---" << std::endl;
    for (int i = 0; i < 4; i++) {
        int root = findMoleculeRoot(i, states);
        int bonds = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        std::cout << "Atom " << i 
                  << " | Root: " << root
                  << " | Parent: " << states[i].parentEntityId
                  << " | Children: " << states[i].childCount
                  << " | Bonds: " << bonds
                  << " | IsTerminal: " << (bonds == 1 ? "YES" : "NO")
                  << std::endl;
    }

    // --- CHECK IF RING CAN CLOSE ---
    std::cout << std::endl << "--- RING CLOSURE CHECK ---" << std::endl;
    std::vector<int> terminals;
    for (int i = 0; i < 4; i++) {
        int bonds = (states[i].parentEntityId != -1 ? 1 : 0) + states[i].childCount;
        if (bonds == 1) {
            terminals.push_back(i);
        }
    }
    
    std::cout << "Found " << terminals.size() << " terminals: ";
    for (int t : terminals) std::cout << t << " ";
    std::cout << std::endl;

    if (terminals.size() >= 2) {
        int t1 = terminals[0];
        int t2 = terminals[1];
        int root1 = findMoleculeRoot(t1, states);
        int root2 = findMoleculeRoot(t2, states);
        float dist = distance(transforms[t1], transforms[t2]);
        
        std::cout << "Terminal pair: " << t1 << " <-> " << t2 << std::endl;
        std::cout << "Same molecule: " << (root1 == root2 ? "YES" : "NO") << std::endl;
        std::cout << "Distance: " << dist << std::endl;
        
        if (root1 == root2 && dist < BOND_AUTO_RANGE * 3.0f) {
            std::cout << std::endl << "=== [SUCCESS] RING CLOSURE CONDITIONS MET ===" << std::endl;
        } else {
            std::cout << std::endl << "=== [FAILURE] Cannot close ring ===" << std::endl;
            if (root1 != root2) std::cout << "  - Different molecules" << std::endl;
            if (dist >= BOND_AUTO_RANGE * 3.0f) std::cout << "  - Too far apart" << std::endl;
        }
    } else {
        std::cout << std::endl << "=== [FAILURE] Not enough terminals to close ring ===" << std::endl;
    }

    return 0;
}
