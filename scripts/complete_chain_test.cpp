/**
 * COMPLETE CHAIN & RING FORMATION TEST
 * 
 * Simulates the FULL flow in detail:
 * 1. 4 isolated Carbon atoms dropped on clay
 * 2. Carbon Affinity attracts them (Phase 36)
 * 3. Linear chain forms with terminal preference (Phase 37)
 * 4. Ring closure when terminals are close (cycle detection)
 * 5. Stability verification (ring maintains shape)
 * 
 * Compile: g++ -o full_test scripts/complete_chain_test.cpp -std=c++17
 * Run: ./full_test
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>

// --- CONFIG (matches current game) ---
const float BOND_AUTO_RANGE = 50.0f;
const float BOND_IDEAL_DIST = 42.0f;
const float AFFINITY_STRENGTH = 15.0f;
const float FOLDING_STRENGTH = 25.0f;
const float BOND_SPRING_K = 8.0f;
const float BOND_DAMPING = 0.92f;
const float DT = 1.0f / 60.0f;
const float DRAG = 0.95f;

// --- ATOM STRUCTURE ---
struct Atom {
    int id;
    float x, y;
    float vx = 0, vy = 0;
    int parentId = -1;      // Parent in tree (-1 = root)
    int childCount = 0;     // Number of children
    int cycleBondId = -1;   // Cycle bond partner (-1 = none)
    bool isClustered = false;
    
    int bondCount() const { 
        return (parentId != -1 ? 1 : 0) + childCount + (cycleBondId != -1 ? 1 : 0); 
    }
    bool isTerminal() const { return bondCount() == 1; }
    bool isIsolated() const { return bondCount() == 0; }
};

// --- UTILITY FUNCTIONS ---
float distance(const Atom& a, const Atom& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

int findRoot(int id, const std::vector<Atom>& atoms) {
    int safetyCounter = 0;
    while (atoms[id].parentId != -1 && safetyCounter < 100) {
        id = atoms[id].parentId;
        safetyCounter++;
    }
    return id;
}

void printState(const std::vector<Atom>& atoms, const std::string& phase) {
    std::cout << "\n=== " << phase << " ===" << std::endl;
    for (const auto& a : atoms) {
        int root = findRoot(a.id, atoms);
        std::cout << "  C" << a.id << ": (" << std::setw(3) << (int)a.x << ", " << std::setw(3) << (int)a.y << ")"
                  << " | Parent: " << (a.parentId == -1 ? "-" : std::to_string(a.parentId))
                  << " | Children: " << a.childCount
                  << " | CycleBond: " << (a.cycleBondId == -1 ? "-" : std::to_string(a.cycleBondId))
                  << " | Root: " << root
                  << " | Bonds: " << a.bondCount()
                  << (a.isTerminal() ? " [TERM]" : "")
                  << (a.isIsolated() ? " [ISOL]" : "")
                  << (a.cycleBondId != -1 ? " [RING]" : "")
                  << std::endl;
    }
}

// --- PHYSICS: Carbon Affinity ---
void applyCarbonAffinity(std::vector<Atom>& atoms) {
    for (size_t i = 0; i < atoms.size(); i++) {
        for (size_t j = i + 1; j < atoms.size(); j++) {
            if (atoms[i].cycleBondId != -1 || atoms[j].cycleBondId != -1) continue;
            
            float dist = distance(atoms[i], atoms[j]);
            if (dist > 30.0f && dist < 150.0f) {
                int rootI = findRoot(i, atoms);
                int rootJ = findRoot(j, atoms);
                float strength = (rootI != rootJ) ? AFFINITY_STRENGTH : 10.0f;
                
                float nx = (atoms[j].x - atoms[i].x) / dist;
                float ny = (atoms[j].y - atoms[i].y) / dist;
                
                atoms[i].vx += nx * strength * DT;
                atoms[i].vy += ny * strength * DT;
                atoms[j].vx -= nx * strength * DT;
                atoms[j].vy -= ny * strength * DT;
            }
        }
    }
}

// --- PHYSICS: Terminal Folding ---
void applyTerminalFolding(std::vector<Atom>& atoms) {
    for (size_t i = 0; i < atoms.size(); i++) {
        for (size_t j = i + 1; j < atoms.size(); j++) {
            if (atoms[i].cycleBondId != -1 || atoms[j].cycleBondId != -1) continue;
            if (!atoms[i].isTerminal() || !atoms[j].isTerminal()) continue;
            
            int rootI = findRoot(i, atoms);
            int rootJ = findRoot(j, atoms);
            if (rootI != rootJ) continue; // Different molecules - skip
            
            float dist = distance(atoms[i], atoms[j]);
            if (dist > 20.0f && dist < 300.0f) {
                float nx = (atoms[j].x - atoms[i].x) / dist;
                float ny = (atoms[j].y - atoms[i].y) / dist;
                
                atoms[i].vx += nx * FOLDING_STRENGTH * DT;
                atoms[i].vy += ny * FOLDING_STRENGTH * DT;
                atoms[j].vx -= nx * FOLDING_STRENGTH * DT;
                atoms[j].vy -= ny * FOLDING_STRENGTH * DT;
            }
        }
    }
}

// --- PHYSICS: Bond Spring Forces ---
void applyBondForces(std::vector<Atom>& atoms) {
    for (size_t i = 0; i < atoms.size(); i++) {
        // Force with parent
        if (atoms[i].parentId != -1) {
            Atom& a = atoms[i];
            Atom& b = atoms[atoms[i].parentId];
            
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < 0.1f) continue;
            
            float strain = dist - BOND_IDEAL_DIST;
            float force = strain * BOND_SPRING_K;
            float nx = dx / dist, ny = dy / dist;
            
            a.vx += nx * force * DT / 12.0f;
            a.vy += ny * force * DT / 12.0f;
            b.vx -= nx * force * DT / 12.0f;
            b.vy -= ny * force * DT / 12.0f;
        }
        
        // Force with cycle bond
        if (atoms[i].cycleBondId != -1 && atoms[i].cycleBondId > (int)i) {
            Atom& a = atoms[i];
            Atom& b = atoms[atoms[i].cycleBondId];
            
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < 0.1f) continue;
            
            float strain = dist - BOND_IDEAL_DIST;
            float force = strain * BOND_SPRING_K;
            float nx = dx / dist, ny = dy / dist;
            
            a.vx += nx * force * DT / 12.0f;
            a.vy += ny * force * DT / 12.0f;
            b.vx -= nx * force * DT / 12.0f;
            b.vy -= ny * force * DT / 12.0f;
        }
    }
}

// --- BONDING: Try to merge two atoms ---
bool tryBond(int i, int j, std::vector<Atom>& atoms) {
    float dist = distance(atoms[i], atoms[j]);
    if (dist > BOND_AUTO_RANGE) return false;
    
    int rootI = findRoot(i, atoms);
    int rootJ = findRoot(j, atoms);
    
    // Same molecule - check for cycle
    if (rootI == rootJ) {
        if (atoms[i].isTerminal() && atoms[j].isTerminal()) {
            // PHASE 38: MINIMUM RING SIZE CHECK (PATH HOPS)
            int hopDistance = 0;
            int current = i;
            std::vector<int> pathI;
            while (current != -1) {
                pathI.push_back(current);
                current = atoms[current].parentId;
            }
            
            current = j;
            bool foundConnection = false;
            while (current != -1) {
                for (size_t p = 0; p < pathI.size(); p++) {
                    if (pathI[p] == current) {
                        hopDistance = p + hopDistance;
                        foundConnection = true;
                        break;
                    }
                }
                if (foundConnection) break;
                hopDistance++;
                current = atoms[current].parentId;
            }

            if (hopDistance < 3) {
                // Ring too small (triangle) - skip
                return false;
            }

            // Cycle closure!
            atoms[i].cycleBondId = j;
            atoms[j].cycleBondId = i;
            std::cout << ">>> [CYCLE] Ring closed: C" << i << " <-> C" << j << " (dist: " << (int)dist << ", hops: " << hopDistance << ")" << std::endl;
            return true;
        }
        return false;
    }
    
    // PHASE 37: Linear chain preference (STRICT)
    int bondsI = atoms[i].bondCount();
    int bondsJ = atoms[j].bondCount();
    if (bondsI >= 2 || bondsJ >= 2) {
        // Either is saturated - skip to force linear chains
        return false;
    }
    
    // EXTRA: Prevent root atoms with children from getting more children
    // Root with children = one end of chain. Isolated should attach to the OTHER end.
    bool iIsRootWithChild = (atoms[i].parentId == -1 && atoms[i].childCount > 0);
    bool jIsRootWithChild = (atoms[j].parentId == -1 && atoms[j].childCount > 0);
    bool iIsIsolated = (bondsI == 0);
    bool jIsIsolated = (bondsJ == 0);
    
    if (iIsRootWithChild && jIsIsolated) return false;
    if (jIsRootWithChild && iIsIsolated) return false;
    
    // Different molecules - merge (j becomes child of i)
    atoms[j].parentId = i;
    atoms[j].isClustered = true;
    atoms[i].childCount++;
    std::cout << ">>> [BOND] Merged: C" << j << " -> C" << i << " (dist: " << (int)dist << ")" << std::endl;
    return true;
}

// --- INTEGRATION ---
void integrate(std::vector<Atom>& atoms) {
    for (auto& a : atoms) {
        a.x += a.vx * DT;
        a.y += a.vy * DT;
        a.vx *= DRAG;
        a.vy *= DRAG;
    }
}

// --- MAIN SIMULATION ---
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  COMPLETE CHAIN & RING FORMATION TEST  " << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create 4 isolated Carbon atoms (spread out)
    std::vector<Atom> atoms(4);
    atoms[0] = {0, 0, 0};
    atoms[1] = {1, 100, 0};
    atoms[2] = {2, 100, 100};
    atoms[3] = {3, 0, 100};
    
    printState(atoms, "INITIAL STATE (4 isolated carbons)");
    
    // Simulation phases
    int tick = 0;
    bool chainComplete = false;
    bool ringClosed = false;
    
    std::cout << "\n--- PHASE 1: Chain Formation ---" << std::endl;
    
    // Run until chain is complete (all in same molecule)
    while (tick < 600 && !ringClosed) {
        // 1. Carbon Affinity
        applyCarbonAffinity(atoms);
        
        // 2. Terminal Folding (only for same molecule)
        applyTerminalFolding(atoms);
        
        // 3. Bond Forces (for existing bonds)
        applyBondForces(atoms);
        
        // 4. Integration
        integrate(atoms);
        
        // 5. Try bonding
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                tryBond(i, j, atoms);
            }
        }
        
        // Check if all in same molecule
        int root0 = findRoot(0, atoms);
        bool allSame = true;
        for (int i = 1; i < 4; i++) {
            if (findRoot(i, atoms) != root0) allSame = false;
        }
        
        if (allSame && !chainComplete) {
            chainComplete = true;
            std::cout << "\n*** CHAIN COMPLETE at tick " << tick << " ***" << std::endl;
            printState(atoms, "CHAIN FORMED");
        }
        
        // Check if ring closed
        for (const auto& a : atoms) {
            if (a.cycleBondId != -1) {
                ringClosed = true;
                break;
            }
        }
        
        tick++;
        
        // Log every 120 ticks for debugging
        if (tick % 120 == 0) {
            std::cout << "\n[Tick " << tick << "] Distances: ";
            for (int i = 0; i < 4; i++) {
                for (int j = i + 1; j < 4; j++) {
                    std::cout << "C" << i << "-C" << j << "=" << (int)distance(atoms[i], atoms[j]) << " ";
                }
            }
            std::cout << std::endl;
        }
    }
    
    // Final state
    printState(atoms, "FINAL STATE");
    
    // Verify results
    std::cout << "\n========================================" << std::endl;
    std::cout << "          VERIFICATION RESULTS          " << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 1. Chain detection
    int uniqueRoots = 0;
    std::vector<bool> counted(4, false);
    for (int i = 0; i < 4; i++) {
        int root = findRoot(i, atoms);
        if (!counted[root]) {
            counted[root] = true;
            uniqueRoots++;
        }
    }
    std::cout << "1. CHAIN FORMED: " << (uniqueRoots == 1 ? "YES ✓" : "NO ✗") << " (molecules: " << uniqueRoots << ")" << std::endl;
    
    // 2. Linear topology (no star patterns)
    bool isLinear = true;
    for (const auto& a : atoms) {
        if (a.childCount > 2 || (a.childCount == 2 && a.parentId == -1 && a.cycleBondId == -1)) {
            isLinear = false;
            std::cout << "   WARNING: C" << a.id << " has " << a.childCount << " children (branching!)" << std::endl;
        }
    }
    std::cout << "2. LINEAR CHAIN: " << (isLinear ? "YES ✓" : "NO ✗ (star pattern detected)") << std::endl;
    
    // 3. Ring closure
    std::cout << "3. RING CLOSED: " << (ringClosed ? "YES ✓" : "NO ✗") << std::endl;
    
    // 4. Total bonds correct (for C4 ring: should be 4 bonds)
    int totalBonds = 0;
    for (const auto& a : atoms) {
        totalBonds += a.bondCount();
    }
    totalBonds /= 2; // Each bond counted twice
    std::cout << "4. TOTAL BONDS: " << totalBonds << " (expected: 4)" << std::endl;
    
    // 5. Final distances
    std::cout << "5. BOND DISTANCES:" << std::endl;
    for (int i = 0; i < 4; i++) {
        if (atoms[i].parentId != -1) {
            float d = distance(atoms[i], atoms[atoms[i].parentId]);
            std::cout << "   C" << i << "-C" << atoms[i].parentId << ": " << (int)d << " (ideal: " << (int)BOND_IDEAL_DIST << ")" << std::endl;
        }
        if (atoms[i].cycleBondId != -1 && atoms[i].cycleBondId > i) {
            float d = distance(atoms[i], atoms[atoms[i].cycleBondId]);
            std::cout << "   C" << i << "-C" << atoms[i].cycleBondId << " [CYCLE]: " << (int)d << " (ideal: " << (int)BOND_IDEAL_DIST << ")" << std::endl;
        }
    }
    
    // Overall result
    std::cout << "\n========================================" << std::endl;
    if (uniqueRoots == 1 && isLinear && ringClosed) {
        std::cout << "  [SUCCESS] Perfect C4 ring formed!    " << std::endl;
    } else if (uniqueRoots == 1 && isLinear) {
        std::cout << "  [PARTIAL] Chain formed, ring pending " << std::endl;
    } else if (uniqueRoots == 1) {
        std::cout << "  [WARNING] Chain formed but branching " << std::endl;
    } else {
        std::cout << "  [FAILURE] Atoms didn't fully connect " << std::endl;
    }
    std::cout << "========================================" << std::endl;
    
    return 0;
}
