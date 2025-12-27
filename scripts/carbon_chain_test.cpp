/**
 * CARBON CHAIN FORMATION TEST (Phase 36)
 * 
 * Simulates the new Carbon Affinity feature:
 * 1. 4 isolated Carbon atoms on Clay
 * 2. Carbon Affinity force pulls them together
 * 3. They bond into a chain
 * 4. Chain terminals close into a ring
 * 
 * Compile: g++ -o carbon_test scripts/carbon_chain_test.cpp -std=c++17
 * Run: ./carbon_test
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// --- CONFIG (matches current game) ---
const float BOND_AUTO_RANGE = 50.0f;
const float BOND_IDEAL_DIST = 42.0f;
const float AFFINITY_STRENGTH = 15.0f;
const float FOLDING_STRENGTH = 25.0f;
const float DT = 1.0f / 60.0f;

// --- MOCK STRUCTURES ---
struct Atom {
    float x, y;
    float vx = 0, vy = 0;
    int parentId = -1;
    int childCount = 0;
    int cycleBondId = -1;
    bool isClustered = false;
    
    int bondCount() const { return (parentId != -1 ? 1 : 0) + childCount; }
    bool isTerminal() const { return bondCount() == 1; }
};

// --- UTILITY ---
int findRoot(int id, const std::vector<Atom>& atoms) {
    while (atoms[id].parentId != -1) {
        id = atoms[id].parentId;
    }
    return id;
}

float distance(const Atom& a, const Atom& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

void applyAttraction(Atom& a, Atom& b, float strength) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    if (dist < 1.0f) return;
    
    float nx = dx / dist;
    float ny = dy / dist;
    
    a.vx += nx * strength * DT;
    a.vy += ny * strength * DT;
    b.vx -= nx * strength * DT;
    b.vy -= ny * strength * DT;
}

void integrate(Atom& a) {
    a.x += a.vx * DT;
    a.y += a.vy * DT;
    a.vx *= 0.95f; // Drag
    a.vy *= 0.95f;
}

bool tryBond(int i, int j, std::vector<Atom>& atoms) {
    if (distance(atoms[i], atoms[j]) > BOND_AUTO_RANGE) return false;
    if (atoms[i].bondCount() >= 4 || atoms[j].bondCount() >= 4) return false;
    
    int rootI = findRoot(i, atoms);
    int rootJ = findRoot(j, atoms);
    
    if (rootI == rootJ) {
        // Same molecule - check for cycle
        if (atoms[i].isTerminal() && atoms[j].isTerminal()) {
            atoms[i].cycleBondId = j;
            atoms[j].cycleBondId = i;
            std::cout << "[CYCLE] Ring closed: " << i << " <-> " << j << std::endl;
            return true;
        }
        return false;
    }
    
    // Different molecules - merge
    atoms[j].parentId = i;
    atoms[j].isClustered = true;
    atoms[i].childCount++;
    std::cout << "[BOND] Merged: " << j << " -> " << i << std::endl;
    return true;
}

// --- MAIN SIMULATION ---
int main() {
    std::cout << "=== CARBON CHAIN FORMATION TEST ===" << std::endl;
    std::cout << "Simulating 4 isolated carbons on clay..." << std::endl << std::endl;

    // Create 4 isolated Carbon atoms in a square pattern
    std::vector<Atom> atoms(4);
    atoms[0] = {0, 0};      // Top-left
    atoms[1] = {80, 0};     // Top-right (far)
    atoms[2] = {80, 80};    // Bottom-right
    atoms[3] = {0, 80};     // Bottom-left

    std::cout << "Initial positions:" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "  C" << i << ": (" << atoms[i].x << ", " << atoms[i].y << ")" << std::endl;
    }

    // Simulate 300 ticks (5 seconds)
    for (int tick = 0; tick < 300; tick++) {
        // PHASE 1: Carbon Affinity - Carbons attract each other
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                if (atoms[i].cycleBondId != -1 || atoms[j].cycleBondId != -1) continue;
                
                float dist = distance(atoms[i], atoms[j]);
                if (dist > 30.0f && dist < 150.0f) {
                    int rootI = findRoot(i, atoms);
                    int rootJ = findRoot(j, atoms);
                    float strength = (rootI != rootJ) ? AFFINITY_STRENGTH : 10.0f;
                    applyAttraction(atoms[i], atoms[j], strength);
                }
            }
        }
        
        // PHASE 2: Terminal Folding - Terminals of same molecule attract
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                if (atoms[i].cycleBondId != -1 || atoms[j].cycleBondId != -1) continue;
                if (!atoms[i].isTerminal() || !atoms[j].isTerminal()) continue;
                
                int rootI = findRoot(i, atoms);
                int rootJ = findRoot(j, atoms);
                if (rootI != rootJ) continue; // Different molecules
                
                float dist = distance(atoms[i], atoms[j]);
                if (dist > 20.0f && dist < 300.0f) {
                    applyAttraction(atoms[i], atoms[j], FOLDING_STRENGTH);
                }
            }
        }
        
        // PHASE 3: Integration
        for (auto& a : atoms) integrate(a);
        
        // PHASE 4: Bonding
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                tryBond(i, j, atoms);
            }
        }
        
        // Log every 60 ticks (1 second)
        if (tick % 60 == 59) {
            std::cout << "\n--- TICK " << tick + 1 << " ---" << std::endl;
            for (int i = 0; i < 4; i++) {
                int root = findRoot(i, atoms);
                std::cout << "  C" << i << ": (" << (int)atoms[i].x << ", " << (int)atoms[i].y 
                          << ") | Root: " << root 
                          << " | Bonds: " << atoms[i].bondCount()
                          << (atoms[i].isTerminal() ? " [TERMINAL]" : "")
                          << (atoms[i].cycleBondId != -1 ? " [RING]" : "")
                          << std::endl;
            }
        }
    }

    // Final check
    std::cout << "\n=== FINAL RESULT ===" << std::endl;
    bool ringFormed = false;
    for (const auto& a : atoms) {
        if (a.cycleBondId != -1) ringFormed = true;
    }
    
    int uniqueRoots = 0;
    std::vector<bool> counted(4, false);
    for (int i = 0; i < 4; i++) {
        int root = findRoot(i, atoms);
        if (!counted[root]) {
            counted[root] = true;
            uniqueRoots++;
        }
    }
    
    std::cout << "Unique molecules: " << uniqueRoots << std::endl;
    std::cout << "Ring formed: " << (ringFormed ? "YES ✓" : "NO ✗") << std::endl;
    
    if (ringFormed) {
        std::cout << "\n[SUCCESS] Carbon chain formed and closed into a ring!" << std::endl;
    } else if (uniqueRoots == 1) {
        std::cout << "\n[PARTIAL] Chain formed but ring didn't close yet." << std::endl;
    } else {
        std::cout << "\n[FAILURE] Carbons didn't fully connect." << std::endl;
    }

    return 0;
}
