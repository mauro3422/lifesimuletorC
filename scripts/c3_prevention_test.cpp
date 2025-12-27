/**
 * C3 RING PREVENTION TEST
 * 
 * Verifies that 3 isolated Carbon atoms dropped on clay 
 * DO NOT form a 3-atom ring (triangle), as it requires
 * path hops >= 3 (4+ atoms chain).
 * 
 * Compile: g++ -o c3_test scripts/c3_prevention_test.cpp -std=c++17
 * Run: ./c3_test
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>

// --- CONFIG ---
const float BOND_AUTO_RANGE = 50.0f;
const float DT = 1.0f / 60.0f;
const float DRAG = 0.95f;

struct Atom {
    int id;
    float x, y;
    float vx = 0, vy = 0;
    int parentId = -1;
    int childCount = 0;
    int cycleBondId = -1;
    
    int bondCount() const { return (parentId != -1 ? 1 : 0) + childCount + (cycleBondId != -1 ? 1 : 0); }
    bool isTerminal() const { return bondCount() == 1; }
    bool isIsolated() const { return bondCount() == 0; }
};

float distance(const Atom& a, const Atom& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

int findRoot(int id, const std::vector<Atom>& atoms) {
    while (atoms[id].parentId != -1) id = atoms[id].parentId;
    return id;
}

bool tryBond(int i, int j, std::vector<Atom>& atoms) {
    float dist = distance(atoms[i], atoms[j]);
    if (dist > BOND_AUTO_RANGE) return false;
    
    int rootI = findRoot(i, atoms);
    int rootJ = findRoot(j, atoms);
    
    if (rootI == rootJ) {
        if (atoms[i].isTerminal() && atoms[j].isTerminal()) {
            // HOP DISTANCE CHECK
            int hopDistance = 0;
            int current = i;
            std::vector<int> pathI;
            while (current != -1) { pathI.push_back(current); current = atoms[current].parentId; }
            current = j;
            bool foundConnection = false;
            while (current != -1) {
                for (size_t p = 0; p < pathI.size(); p++) {
                    if (pathI[p] == current) { hopDistance = p + hopDistance; foundConnection = true; break; }
                }
                if (foundConnection) break;
                hopDistance++;
                current = atoms[current].parentId;
            }

            if (hopDistance < 3) {
                std::cout << "! [REJECTED] Ring too small: C" << i << "-C" << j << " (hops: " << hopDistance << ")" << std::endl;
                return false;
            }

            atoms[i].cycleBondId = j;
            atoms[j].cycleBondId = i;
            std::cout << ">>> [CYCLE] Ring closed! C" << i << " <-> C" << j << " (hops: " << hopDistance << ")" << std::endl;
            return true;
        }
        return false;
    }
    
    // Simple merge for test
    atoms[j].parentId = i;
    atoms[i].childCount++;
    std::cout << ">>> [BOND] Merged C" << j << " -> C" << i << std::endl;
    return true;
}

int main() {
    std::cout << "--- C3 RING PREVENTION TEST ---" << std::endl;
    std::vector<Atom> atoms(3);
    atoms[0] = {0, 0, 0};
    atoms[1] = {1, 40, 0};
    atoms[2] = {2, 20, 30}; // Triangle shape, all within range
    
    // Try to form chain C2-C1-C0
    tryBond(1, 0, atoms); // C1 -> C0
    tryBond(2, 1, atoms); // C2 -> C1
    
    // Now C2 and C0 are terminals of C2-C1-C0
    std::cout << "Distance C0-C2: " << distance(atoms[0], atoms[2]) << std::endl;
    
    bool ringCreated = tryBond(0, 2, atoms);
    
    std::cout << "\nRESULT: " << (ringCreated ? "FAIL (Ring formed!) ✗" : "SUCCESS (Ring prevented) ✓") << std::endl;
    
    return ringCreated ? 1 : 0;
}
