/**
 * test_integration_molecule.cpp
 * Generalized molecule formation test with session logging
 * 
 * Usage: ./test_molecule.exe [structure_name] [flags]
 * Example: ./test_molecule.exe carbon_hexagon
 *          ./test_molecule.exe carbon_hexagon --animation --random
 * 
 * Flags:
 *   --animation    Run 1000 frames (default: 300) for animation diagnostics
 *   --random       Spawn atoms at random positions (default: circle)
 * 
 * Features:
 * - Uses REAL physics/bonding systems (no mocks)
 * - Saves ASCII grid + coordinates to logs/session_*.log
 * - Validates against structure definition from structures.json
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

// Real system includes (NO MOCKS)
#include "../ecs/components.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "../physics/BondingSystem.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../physics/AutonomousBonding.hpp"
#include "../physics/StructureDetector.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../world/EnvironmentManager.hpp"
#include "../world/zones/ClayZone.hpp"
#include "../core/Config.hpp"

// ============================================================================
// SESSION LOGGER
// ============================================================================
class SessionLogger {
public:
    std::ofstream file;
    std::string filename;
    
    SessionLogger(const std::string& structureName) {
        // Generate timestamp filename
        std::time_t now = std::time(nullptr);
        std::tm* ltm = std::localtime(&now);
        std::ostringstream oss;
        oss << "logs/session_" << structureName << "_"
            << std::put_time(ltm, "%Y%m%d_%H%M%S") << ".log";
        filename = oss.str();
        file.open(filename);
        
        if (file.is_open()) {
            file << "=== MOLECULE INTEGRATION TEST SESSION ===" << std::endl;
            file << "Structure: " << structureName << std::endl;
            file << "Timestamp: " << std::put_time(ltm, "%Y-%m-%d %H:%M:%S") << std::endl;
            file << "===========================================" << std::endl << std::endl;
        }
    }
    
    ~SessionLogger() {
        if (file.is_open()) {
            file << "\n=== SESSION END ===" << std::endl;
            file.close();
        }
    }
    
    void log(const std::string& msg) {
        if (file.is_open()) {
            file << msg << std::endl;
        }
        std::cout << msg << std::endl;
    }
    
    void logGrid(const std::vector<TransformComponent>& transforms,
                 const std::vector<StateComponent>& states,
                 float centerX, float centerY, int gridSize = 100) {
        std::ostringstream oss;
        oss << "\n=== POSITION GRID (centered at " << centerX << ", " << centerY << ") ===" << std::endl;
        
        // Track which atoms we've already displayed
        std::vector<bool> shown(transforms.size(), false);
        
        // Header row
        oss << "      ";
        for (int gx = -gridSize/2; gx <= gridSize/2; gx += 10) {
            oss << std::setw(4) << (int)(centerX + gx);
        }
        oss << std::endl;
        
        // Grid rows
        for (int gy = -gridSize/2; gy <= gridSize/2; gy += 10) {
            oss << std::setw(5) << gy << " |";
            for (int gx = -gridSize/2; gx <= gridSize/2; gx += 10) {
                float cellX = centerX + gx;
                float cellY = centerY + gy;
                
                bool found = false;
                for (size_t i = 1; i < transforms.size() && !found; i++) {
                    if (shown[i]) continue;  // Skip already displayed atoms
                    float dx = transforms[i].x - cellX;
                    float dy = transforms[i].y - cellY;
                    if (std::abs(dx) <= 6 && std::abs(dy) <= 6) {
                        if (states[i].isInRing) {
                            oss << " [" << i << "]";
                        } else {
                            oss << "  " << i << " ";
                        }
                        found = true;
                        shown[i] = true;  // Mark as displayed
                    }
                }
                if (!found) oss << "   .";
            }
            oss << "|" << std::endl;
        }
        
        log(oss.str());
    }
    
    void logAtomTable(const std::vector<TransformComponent>& transforms,
                      const std::vector<StateComponent>& states,
                      float expectedCenterX, float expectedCenterY,
                      float expectedRadius) {
        std::ostringstream oss;
        oss << "\nPer-Atom Status:" << std::endl;
        oss << "  ID       X       Y  InRing  RSize  Cycle  Parent    Dist  Status" << std::endl;
        oss << "----------------------------------------------------------------------" << std::endl;
        
        int correctCount = 0;
        int ringCount = 0;
        
        for (size_t i = 1; i < transforms.size(); i++) {
            float dx = transforms[i].x - expectedCenterX;
            float dy = transforms[i].y - expectedCenterY;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            bool distOk = std::abs(dist - expectedRadius) < 10.0f;
            bool inRing = states[i].isInRing;
            
            if (inRing) ringCount++;
            if (inRing && distOk) correctCount++;
            
            oss << std::setw(4) << i
                << std::setw(8) << std::fixed << std::setprecision(1) << transforms[i].x
                << std::setw(8) << transforms[i].y
                << std::setw(8) << (inRing ? "YES" : "NO")
                << std::setw(7) << states[i].ringSize
                << std::setw(7) << states[i].cycleBondId
                << std::setw(8) << states[i].parentEntityId
                << std::setw(8) << dist
                << "    " << (inRing && distOk ? "OK" : "FAIL") << std::endl;
        }
        
        oss << "----------------------------------------------------------------------" << std::endl;
        oss << "Atoms in ring: " << ringCount << "/" << (transforms.size() - 1) << std::endl;
        oss << "Correct positions: " << correctCount << "/" << (transforms.size() - 1) << std::endl;
        
        log(oss.str());
    }
    
    void logFramePositions(int frame, const std::vector<TransformComponent>& transforms,
                           const std::vector<StateComponent>& states) {
        std::ostringstream oss;
        oss << "  [Frame " << std::setw(4) << frame << "] ";
        
        // Count various states
        int ringCount = 0, clusteredCount = 0, bondCount = 0;
        float centroidX = 0, centroidY = 0;
        float totalDocking = 0;  // Track animation progress
        
        for (size_t i = 1; i < states.size(); i++) {
            if (states[i].isInRing) {
                ringCount++;
                totalDocking += states[i].dockingProgress;
            }
            if (states[i].isClustered) {
                clusteredCount++;
                bondCount++;  // Each clustered atom has a bond to parent
            }
            centroidX += transforms[i].x;
            centroidY += transforms[i].y;
        }
        
        int numAtoms = (int)states.size() - 1;
        if (numAtoms > 0) {
            centroidX /= numAtoms;
            centroidY /= numAtoms;
        }
        
        // Calculate max distance from centroid (spread indicator)
        float maxDist = 0;
        for (size_t i = 1; i < transforms.size(); i++) {
            float dx = transforms[i].x - centroidX;
            float dy = transforms[i].y - centroidY;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist > maxDist) maxDist = dist;
        }
        
        // Average docking progress for ring atoms (0.0 = start, 1.0 = complete)
        float avgDocking = ringCount > 0 ? (totalDocking / ringCount) : 0.0f;
        
        oss << "Ring:" << ringCount << "/" << numAtoms
            << " Bonds:" << bondCount << "/" << numAtoms
            << " Dock:" << std::fixed << std::setprecision(0) << (avgDocking * 100) << "%"
            << " Spread:" << std::fixed << std::setprecision(0) << maxDist << "px";
        
        // Log positions for animation snapshots
        oss << "\n    Pos:";
        for (size_t i = 1; i < transforms.size(); i++) {
            char marker = states[i].isInRing ? '*' : (states[i].isClustered ? '+' : ' ');
            oss << " [" << i << marker << "](" << std::fixed << std::setprecision(0) 
                << transforms[i].x << "," << transforms[i].y << ")";
        }
        
        log(oss.str());
    }
};

// ============================================================================
// EXPECTED GEOMETRY
// ============================================================================
struct ExpectedGeometry {
    float centerX, centerY;
    float radius;
    int atomCount;
    std::string structureName;
};

ExpectedGeometry getExpectedGeometry(const std::string& structureName) {
    const auto& registry = StructureRegistry::getInstance();
    
    ExpectedGeometry geo;
    geo.structureName = structureName;
    geo.centerX = -800.0f;  // Clay Zone center
    geo.centerY = 0.0f;
    geo.atomCount = 6;  // Default
    geo.radius = 42.0f; // Default (will be calculated)
    
    for (const auto& d : registry.getAllStructures()) {
        if (d.name == structureName) {
            geo.atomCount = d.atomCount;
            // Calculate radius from bond distance and polygon
            float angleStep = 2.0f * 3.14159f / d.atomCount;
            geo.radius = Config::BOND_IDEAL_DIST / (2.0f * std::sin(angleStep / 2.0f));
            break;
        }
    }
    
    return geo;
}

// ============================================================================
// MAIN TEST
// ============================================================================
int main(int argc, char* argv[]) {
    std::string structureName = "carbon_hexagon";  // Default
    bool animationMode = false;
    bool randomMode = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--animation" || arg == "-a") {
            animationMode = true;
        } else if (arg == "--random" || arg == "-r") {
            randomMode = true;
        } else if (arg[0] != '-') {
            structureName = arg;
        }
    }
    
    SessionLogger log(structureName);
    
    log.log("=== MOLECULE INTEGRATION TEST ===");
    log.log("Testing: " + structureName);
    if (animationMode) log.log("Mode: ANIMATION (1000 frames)");
    if (randomMode) log.log("Mode: RANDOM spawn positions");
    log.log("Using REAL physics systems (no mocks)");
    log.log("");
    
    // Initialize real systems BEFORE physics (same as in main.cpp)
    ChemistryDatabase::getInstance().initialize();  // Load elements FIRST
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    // ANIMATION MODE: Disable instant formation to use gradual docking
    if (animationMode) {
        StructureRegistry::getInstance().setInstantFormation(false);
        log.log("[ANIMATION] Disabled instantFormation - using gradual docking");
    }
    
    ExpectedGeometry expected = getExpectedGeometry(structureName);
    
    log.log("[OK] Structure: " + expected.structureName + 
            " (atomCount=" + std::to_string(expected.atomCount) + 
            ", radius=" + std::to_string((int)expected.radius) + ")");
    
    // ECS data
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder (index 0)
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false});
    
    // Spawn atoms
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(-50.0f, 50.0f);  // Scatter ±50px (within bonding range)
    std::uniform_real_distribution<float> distY(-50.0f, 50.0f);
    
    if (randomMode) {
        log.log("Spawning " + std::to_string(expected.atomCount) + 
                " atoms at RANDOM positions in Clay Zone");
        
        std::ostringstream initialPos;
        initialPos << "Initial positions:";
        
        for (int i = 0; i < expected.atomCount; i++) {
            float x = expected.centerX + distX(gen);
            float y = expected.centerY + distY(gen);
            
            transforms.push_back({x, y, 0, 0, 0, 0, 0});
            atoms.push_back({6, 0});  // Carbon
            states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
            
            initialPos << " [" << (i+1) << "](" << std::fixed << std::setprecision(1) << x << "," << y << ")";
        }
        log.log(initialPos.str());
    } else {
        // Spawn in circle (default)
        float spawnRadius = 35.0f;
        log.log("Spawning " + std::to_string(expected.atomCount) + 
                " atoms in CIRCLE at Clay Zone center, radius=" + std::to_string((int)spawnRadius));
        
        for (int i = 0; i < expected.atomCount; i++) {
            float angle = i * (2.0f * 3.14159f / expected.atomCount);
            float x = expected.centerX + std::cos(angle) * spawnRadius;
            float y = expected.centerY + std::sin(angle) * spawnRadius;
            
            transforms.push_back({x, y, 0, 0, 0, 0, 0});
            atoms.push_back({6, 0});  // Carbon
            states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        }
    }
    
    log.log("Initial state:");
    // Use larger grid for random mode (250px to show scattered atoms)
    int gridSize = randomMode ? 250 : 100;
    log.logGrid(transforms, states, expected.centerX, expected.centerY, gridSize);
    
    // Initialize physics with Clay Zone (CORRECT API)
    PhysicsEngine physics;
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    log.log("Clay Zone added at (-1200, -400) size 800x800");
    log.log("Center of Clay Zone: (-800, 0)");
    
    // Run physics simulation (300 default, 1000 for --animation mode)
    const int MAX_FRAMES = animationMode ? 1000 : 300;
    const int LOG_INTERVAL = animationMode ? 5 : 200;  // Every 5 frames for animation detail
    const float dt = 1.0f / 60.0f;
    
    log.log("\nRunning " + std::to_string(MAX_FRAMES) + " physics frames...");
    
    int ringFormedFrame = -1;
    
    for (int frame = 0; frame < MAX_FRAMES; frame++) {
        physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
        
        // Log every LOG_INTERVAL frames
        if (frame % LOG_INTERVAL == 0) {
            log.logFramePositions(frame, transforms, states);
        }
        
        // Check for ring formation
        if (ringFormedFrame < 0) {
            int ringCount = 0;
            for (size_t i = 1; i < states.size(); i++) {
                if (states[i].isInRing) ringCount++;
            }
            if (ringCount == expected.atomCount) {
                ringFormedFrame = frame;
                log.log("\n*** RING FORMED at frame " + std::to_string(frame) + " ***");
                log.logGrid(transforms, states, expected.centerX, expected.centerY);
            }
        }
    }
    
    // Final state
    log.log("\n=== FINAL STATE ===");
    
    // Calculate actual centroid of formed ring (for random spawn mode)
    float actualCenterX = 0, actualCenterY = 0;
    int ringCount = 0;
    for (size_t i = 1; i < states.size(); i++) {
        if (states[i].isInRing) {
            actualCenterX += transforms[i].x;
            actualCenterY += transforms[i].y;
            ringCount++;
        }
    }
    if (ringCount > 0) {
        actualCenterX /= ringCount;
        actualCenterY /= ringCount;
    }
    
    log.logGrid(transforms, states, actualCenterX, actualCenterY);
    log.logAtomTable(transforms, states, actualCenterX, actualCenterY, expected.radius);
    
    // Validation (using actual centroid)
    int correctCount = 0;
    for (size_t i = 1; i < states.size(); i++) {
        if (states[i].isInRing) {
            float dx = transforms[i].x - actualCenterX;
            float dy = transforms[i].y - actualCenterY;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (std::abs(dist - expected.radius) < 10.0f) correctCount++;
        }
    }
    
    bool passed = (ringCount == expected.atomCount && correctCount == expected.atomCount);
    
    log.log("\n=== RESULT: " + std::string(passed ? "PASSED" : "FAILED") + " ===");
    log.log("Session log saved to: " + log.filename);
    
    return passed ? 0 : 1;
}
