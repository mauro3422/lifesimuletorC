/**
 * RING STABILITY TEST (Phase 36)
 * 
 * Verifies that once a ring forms:
 * 1. Atoms stay connected (bonds don't break)
 * 2. Atoms can vibrate but maintain formation
 * 3. Structure is stable for visual display
 * 
 * Compile: g++ -o stability_test scripts/ring_stability_test.cpp -std=c++17
 * Run: ./stability_test
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// --- CONFIG (matches current game) ---
const float BOND_IDEAL_DIST = 42.0f;
const float BOND_SPRING_K = 8.0f;
const float BOND_DAMPING = 0.92f;
const float BOND_BREAK_STRESS = 180.0f;
const float DT = 1.0f / 60.0f;
const float DRAG = 0.95f;

// --- STRUCTURES ---
struct Atom {
    float x, y;
    float vx = 0, vy = 0;
    int bonds[4] = {-1, -1, -1, -1};
    int bondCount = 0;
    
    void addBond(int other) {
        if (bondCount < 4) bonds[bondCount++] = other;
    }
};

// --- PHYSICS ---
float distance(const Atom& a, const Atom& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

float applyBondForce(Atom& a, Atom& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    if (dist < 0.1f) return 0;
    
    // Spring force
    float strain = dist - BOND_IDEAL_DIST;
    float force = strain * BOND_SPRING_K;
    
    float nx = dx / dist;
    float ny = dy / dist;
    
    // Damping
    float rvx = b.vx - a.vx;
    float rvy = b.vy - a.vy;
    float damp = (rvx * nx + rvy * ny) * BOND_DAMPING;
    
    a.vx += (nx * force + nx * damp) * DT / 12.0f; // Mass = 12 (Carbon)
    a.vy += (ny * force + ny * damp) * DT / 12.0f;
    b.vx -= (nx * force + nx * damp) * DT / 12.0f;
    b.vy -= (ny * force + ny * damp) * DT / 12.0f;
    
    return std::abs(strain);
}

// --- MAIN SIMULATION ---
int main() {
    std::cout << "=== RING STABILITY TEST ===" << std::endl;
    std::cout << "Simulating a pre-formed C4 ring..." << std::endl << std::endl;

    // Create a pre-formed ring (already bonded)
    std::vector<Atom> atoms(4);
    float side = BOND_IDEAL_DIST;
    
    // Square formation
    atoms[0] = {0, 0};
    atoms[1] = {side, 0};
    atoms[2] = {side, side};
    atoms[3] = {0, side};
    
    // Establish bonds (0-1, 1-2, 2-3, 3-0)
    atoms[0].addBond(1); atoms[1].addBond(0);
    atoms[1].addBond(2); atoms[2].addBond(1);
    atoms[2].addBond(3); atoms[3].addBond(2);
    atoms[3].addBond(0); atoms[0].addBond(3);
    
    std::cout << "Initial ring formed with BOND_IDEAL_DIST = " << BOND_IDEAL_DIST << std::endl;
    std::cout << "Testing stability over 600 ticks (10 seconds)..." << std::endl;
    
    // Apply a disturbance (push atom 0)
    atoms[0].vx = 50.0f;
    atoms[0].vy = 30.0f;
    std::cout << "\nApplied disturbance to atom 0..." << std::endl;
    
    float maxStrain = 0;
    float avgStrain = 0;
    int strainSamples = 0;
    bool broken = false;
    
    for (int tick = 0; tick < 600; tick++) {
        // Apply bond forces
        for (int i = 0; i < 4; i++) {
            for (int b = 0; b < atoms[i].bondCount; b++) {
                int j = atoms[i].bonds[b];
                if (j > i) { // Avoid double processing
                    float strain = applyBondForce(atoms[i], atoms[j]);
                    if (strain > maxStrain) maxStrain = strain;
                    avgStrain += strain;
                    strainSamples++;
                    
                    if (strain > BOND_BREAK_STRESS) {
                        broken = true;
                        std::cout << "[BROKEN] Bond " << i << "-" << j << " at tick " << tick << "!" << std::endl;
                    }
                }
            }
        }
        
        // Integration + Drag
        for (auto& a : atoms) {
            a.x += a.vx * DT;
            a.y += a.vy * DT;
            a.vx *= DRAG;
            a.vy *= DRAG;
        }
        
        // Log every 120 ticks
        if (tick % 120 == 119) {
            float d01 = distance(atoms[0], atoms[1]);
            float d12 = distance(atoms[1], atoms[2]);
            float d23 = distance(atoms[2], atoms[3]);
            float d30 = distance(atoms[3], atoms[0]);
            
            std::cout << "\nTick " << tick + 1 << ":" << std::endl;
            std::cout << "  Distances: " << (int)d01 << ", " << (int)d12 << ", " << (int)d23 << ", " << (int)d30 << std::endl;
            std::cout << "  Deviation from ideal: " << (int)(d01 - BOND_IDEAL_DIST) << ", " 
                      << (int)(d12 - BOND_IDEAL_DIST) << ", " << (int)(d23 - BOND_IDEAL_DIST) << ", " 
                      << (int)(d30 - BOND_IDEAL_DIST) << std::endl;
        }
    }
    
    avgStrain /= strainSamples;
    
    std::cout << "\n=== STABILITY RESULTS ===" << std::endl;
    std::cout << "Max strain observed: " << (int)maxStrain << " (break threshold: " << (int)BOND_BREAK_STRESS << ")" << std::endl;
    std::cout << "Avg strain: " << avgStrain << std::endl;
    std::cout << "Ring broken: " << (broken ? "YES ✗" : "NO ✓") << std::endl;
    
    // Final positions
    std::cout << "\nFinal positions:" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "  C" << i << ": (" << (int)atoms[i].x << ", " << (int)atoms[i].y << ")" << std::endl;
    }
    
    // Check if still a valid ring (all distances within tolerance)
    float tolerance = 15.0f;
    bool valid = true;
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        float d = distance(atoms[i], atoms[j]);
        if (std::abs(d - BOND_IDEAL_DIST) > tolerance) {
            valid = false;
            std::cout << "[WARNING] Bond " << i << "-" << j << " deviated: " << (int)d << " vs ideal " << (int)BOND_IDEAL_DIST << std::endl;
        }
    }
    
    if (!broken && valid) {
        std::cout << "\n[SUCCESS] Ring is STABLE and maintains formation!" << std::endl;
    } else if (!broken) {
        std::cout << "\n[PARTIAL] Ring didn't break but shape is distorted." << std::endl;
    } else {
        std::cout << "\n[FAILURE] Ring broke - need stronger bonds or more damping." << std::endl;
    }

    return 0;
}
