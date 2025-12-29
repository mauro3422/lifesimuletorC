/**
 * test_integration_hexagon.cpp
 * 
 * GRID-BASED INTEGRATION TEST for hexagon formation.
 * Simulates physics frames and validates final structure matches expected configuration.
 * 
 * This test:
 * 1. Sets up 6 carbons in a loose circular arrangement
 * 2. Runs physics simulation for N frames
 * 3. Validates final structure using a grid-based position check
 * 4. Reports exact deviations from expected hexagon geometry
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

#include "raylib.h"

#include "../ecs/components.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "../physics/BondingSystem.hpp"
#include "../world/zones/ClayZone.hpp"

// ============================================================================
// EXPECTED STRUCTURE DEFINITION
// ============================================================================

struct ExpectedAtom {
    int id;
    float expectedX;
    float expectedY;
    float tolerance;
    bool shouldBeInRing;
    int expectedRingSize;
};

struct ExpectedHexagon {
    float centerX;
    float centerY;
    float radius;  // Distance from center to each vertex
    std::vector<ExpectedAtom> atoms;
    
    void calculateExpectedPositions() {
        atoms.clear();
        for (int i = 0; i < 6; i++) {
            float angle = i * (2.0f * 3.14159f / 6.0f);
            float x = centerX + std::cos(angle) * radius;
            float y = centerY + std::sin(angle) * radius;
            atoms.push_back({i + 1, x, y, 5.0f, true, 6});  // ID starts at 1 (0 is player)
        }
    }
};

// ============================================================================
// GRID VISUALIZATION
// ============================================================================

void printGrid(const std::vector<TransformComponent>& transforms,
               const std::vector<StateComponent>& states,
               float centerX, float centerY, float gridSize = 100.0f, int cells = 10) {
    
    std::cout << "\n=== POSITION GRID (centered at " << centerX << ", " << centerY << ") ===" << std::endl;
    
    float cellSize = gridSize / cells;
    float halfGrid = gridSize / 2.0f;
    
    // Header row
    std::cout << "      ";
    for (int x = 0; x < cells; x++) {
        std::cout << std::setw(4) << (int)(centerX - halfGrid + x * cellSize);
    }
    std::cout << std::endl;
    
    // Grid cells
    for (int y = 0; y < cells; y++) {
        float rowY = centerY - halfGrid + y * cellSize;
        std::cout << std::setw(5) << (int)rowY << " |";
        
        for (int x = 0; x < cells; x++) {
            float cellX = centerX - halfGrid + x * cellSize;
            float cellEndX = cellX + cellSize;
            float cellEndY = rowY + cellSize;
            
            // Find atoms in this cell
            std::string cellContent = ".";
            for (size_t i = 1; i < transforms.size(); i++) {  // Skip player
                if (transforms[i].x >= cellX && transforms[i].x < cellEndX &&
                    transforms[i].y >= rowY && transforms[i].y < cellEndY) {
                    if (states[i].isInRing) {
                        cellContent = "[" + std::to_string(i) + "]";
                    } else {
                        cellContent = " " + std::to_string(i) + " ";
                    }
                    break;
                }
            }
            std::cout << std::setw(4) << cellContent;
        }
        std::cout << "|" << std::endl;
    }
    std::cout << std::endl;
}

// ============================================================================
// VALIDATION
// ============================================================================

bool validateHexagonFormation(const std::vector<TransformComponent>& transforms,
                              const std::vector<StateComponent>& states,
                              const ExpectedHexagon& expected) {
    
    std::cout << "\n=== VALIDATION RESULTS ===" << std::endl;
    
    bool allPassed = true;
    int ringCount = 0;
    int correctPositions = 0;
    
    // Calculate actual centroid
    float actualCX = 0, actualCY = 0;
    for (size_t i = 1; i < transforms.size() && i <= 6; i++) {
        actualCX += transforms[i].x;
        actualCY += transforms[i].y;
    }
    actualCX /= 6.0f;
    actualCY /= 6.0f;
    
    std::cout << "Expected center: (" << expected.centerX << ", " << expected.centerY << ")" << std::endl;
    std::cout << "Actual center:   (" << actualCX << ", " << actualCY << ")" << std::endl;
    
    // Calculate actual radius (average distance from center)
    float actualRadius = 0;
    for (size_t i = 1; i <= 6; i++) {
        float dx = transforms[i].x - actualCX;
        float dy = transforms[i].y - actualCY;
        actualRadius += std::sqrt(dx*dx + dy*dy);
    }
    actualRadius /= 6.0f;
    
    std::cout << "Expected radius: " << expected.radius << std::endl;
    std::cout << "Actual radius:   " << actualRadius << std::endl;
    
    std::cout << "\nPer-Atom Status:" << std::endl;
    std::cout << std::setw(4) << "ID" << std::setw(10) << "X" << std::setw(10) << "Y" 
              << std::setw(8) << "InRing" << std::setw(8) << "RSize" 
              << std::setw(8) << "Cycle" << std::setw(8) << "Parent"
              << std::setw(10) << "Dist" << std::setw(8) << "Status" << std::endl;
    std::cout << std::string(74, '-') << std::endl;
    
    for (size_t i = 1; i <= 6 && i < transforms.size(); i++) {
        float dx = transforms[i].x - actualCX;
        float dy = transforms[i].y - actualCY;
        float dist = std::sqrt(dx*dx + dy*dy);
        
        bool inRingOK = states[i].isInRing == true;
        bool ringSizeOK = states[i].ringSize == 6;
        bool radiusOK = std::abs(dist - expected.radius) < 5.0f;
        
        if (states[i].isInRing) ringCount++;
        if (radiusOK) correctPositions++;
        
        std::string status = (inRingOK && ringSizeOK && radiusOK) ? "OK" : "FAIL";
        if (!inRingOK || !ringSizeOK || !radiusOK) allPassed = false;
        
        std::cout << std::setw(4) << i 
                  << std::setw(10) << std::fixed << std::setprecision(1) << transforms[i].x
                  << std::setw(10) << transforms[i].y
                  << std::setw(8) << (states[i].isInRing ? "YES" : "NO")
                  << std::setw(8) << states[i].ringSize
                  << std::setw(8) << states[i].cycleBondId
                  << std::setw(8) << states[i].parentEntityId
                  << std::setw(10) << dist
                  << std::setw(8) << status << std::endl;
    }
    
    std::cout << std::string(66, '-') << std::endl;
    std::cout << "Atoms in ring: " << ringCount << "/6" << std::endl;
    std::cout << "Correct positions: " << correctPositions << "/6" << std::endl;
    
    return allPassed;
}

// ============================================================================
// MAIN TEST
// ============================================================================

int main() {
    std::cout << "=== HEXAGON INTEGRATION TEST ===" << std::endl;
    std::cout << "Testing full physics simulation with grid validation" << std::endl << std::endl;
    
    // Initialize systems
    ChemistryDatabase::getInstance().reload();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    // Verify hexagon structure loaded
    const StructureDefinition* hexDef = StructureRegistry::getInstance().findMatch(6, 6);
    if (hexDef) {
        std::cout << "[OK] Structure registry has hexagon: " << hexDef->name 
                  << " (atomCount=" << hexDef->atomCount << ", radius=" 
                  << hexDef->getIdealOffsets(Config::BOND_IDEAL_DIST)[0].x << ")" << std::endl;
    } else {
        std::cout << "[ERROR] Hexagon structure NOT in registry!" << std::endl;
    }
    
    // Setup expected hexagon
    ExpectedHexagon expected;
    expected.centerX = -800.0f;
    expected.centerY = 0.0f;
    expected.radius = Config::BOND_IDEAL_DIST;  // After formation, should match bond distance
    expected.calculateExpectedPositions();
    
    // Initialize world state (simulate World::initializeTestMode)
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player (ID 0)
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
    
    // 6 Carbons in CIRCLE - terminals will be adjacent for ring closure
    // With linear chain enforcement, bonds will form sequentially: 1-2, 2-3, 3-4, 4-5, 5-6
    // Then cycle closes: 6-1 (because they're adjacent in circle)
    float radius = 35.0f;  // Small radius to ensure all neighbors are in range
    std::cout << "Spawning 6 carbons in CIRCLE at Clay Zone center, radius=" << radius << std::endl;
    
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);  // 60 degrees apart
        float x = expected.centerX + std::cos(angle) * radius;
        float y = expected.centerY + std::sin(angle) * radius;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
    }
    
    std::cout << "Initial state: 6 carbons in circle, radius=" << radius << std::endl;
    printGrid(transforms, states, expected.centerX, expected.centerY);
    
    // Initialize physics with Clay Zone
    PhysicsEngine physics;
    
    // Add Clay Zone to environment (same as main.cpp)
    // Clay Zone: { -1200, -400, 800, 800 } - center at (-800, 0)
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    std::cout << "Clay Zone added at (-1200, -400) size 800x800" << std::endl;
    std::cout << "Center of Clay Zone: (-800, 0)" << std::endl;
    
    // Simulate N frames
    const int SIMULATION_FRAMES = 300;  // 5 seconds at 60fps
    const float dt = 1.0f / 60.0f;
    
    std::cout << "Running " << SIMULATION_FRAMES << " physics frames..." << std::endl;
    
    for (int frame = 0; frame < SIMULATION_FRAMES; frame++) {
        physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
        
        // Progress update every second
        if (frame > 0 && frame % 60 == 0) {
            int inRing = 0;
            int withCycleBond = 0;
            for (size_t i = 1; i <= 6; i++) {
                if (states[i].isInRing) inRing++;
                if (states[i].cycleBondId >= 0) withCycleBond++;
            }
            std::cout << "  Frame " << frame << ": " << inRing << "/6 in ring, " 
                      << withCycleBond << "/6 have cycleBond" << std::endl;
        }
        
        // Detailed output when ring just forms
        if (frame == 1 || (frame > 1 && states[1].isInRing && !states[1].isInRing)) {
            // First frame where ring forms - print positions
            std::cout << "  [SNAP CHECK Frame " << frame << "] Positions after physics:" << std::endl;
            for (size_t i = 1; i <= 6; i++) {
                std::cout << "    Atom " << i << ": (" << transforms[i].x << ", " << transforms[i].y << ")" << std::endl;
            }
        }
    }
    
    std::cout << "\nFinal state after simulation:" << std::endl;
    printGrid(transforms, states, expected.centerX, expected.centerY);
    
    // Validate
    bool passed = validateHexagonFormation(transforms, states, expected);
    
    std::cout << std::endl;
    if (passed) {
        std::cout << "=== TEST PASSED ===" << std::endl;
        return 0;
    } else {
        std::cout << "=== TEST FAILED ===" << std::endl;
        return 1;
    }
}
