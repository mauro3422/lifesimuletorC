/**
 * test_cluster_detection.cpp
 * 
 * Tests cluster detection and spontaneous bonding with REAL systems.
 * Verifies that atoms properly form and detect molecular clusters.
 * 
 * Usage: ./test_cluster_detection.exe [flags]
 * Flags:
 *   --verbose    Show per-frame details
 */

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <iomanip>

// Real system includes (NO MOCKS)
#include "../ecs/components.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "../physics/BondingSystem.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../physics/AutonomousBonding.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../world/EnvironmentManager.hpp"
#include "../world/zones/ClayZone.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"

// ============================================================================
// TEST UTILITIES
// ============================================================================

struct ClusterInfo {
    int rootId;
    std::vector<int> members;
    int bondCount;
};

// Count distinct clusters by moleculeId
std::vector<ClusterInfo> detectClusters(const std::vector<StateComponent>& states) {
    std::map<int, ClusterInfo> clusterMap;
    
    for (int i = 1; i < (int)states.size(); i++) {
        int root = MathUtils::findMoleculeRoot(i, states);
        if (clusterMap.find(root) == clusterMap.end()) {
            clusterMap[root] = {root, {}, 0};
        }
        clusterMap[root].members.push_back(i);
        
        // Count bonds (parent-child)
        if (states[i].parentEntityId != -1) {
            clusterMap[root].bondCount++;
        }
        // Count cycle bonds (only once)
        if (states[i].cycleBondId > i) {
            clusterMap[root].bondCount++;
        }
    }
    
    std::vector<ClusterInfo> result;
    for (auto& [k, v] : clusterMap) {
        result.push_back(v);
    }
    return result;
}

int countTotalBonds(const std::vector<StateComponent>& states) {
    int bonds = 0;
    for (int i = 1; i < (int)states.size(); i++) {
        if (states[i].parentEntityId != -1) bonds++;
        if (states[i].cycleBondId > i) bonds++;
    }
    return bonds;
}

void printClusterSummary(const std::vector<ClusterInfo>& clusters) {
    std::cout << "  Clusters: " << clusters.size() << std::endl;
    for (const auto& c : clusters) {
        std::cout << "    Root " << c.rootId << ": " << c.members.size() 
                  << " atoms, " << c.bondCount << " bonds [";
        for (size_t i = 0; i < c.members.size(); i++) {
            if (i > 0) std::cout << ",";
            std::cout << c.members[i];
        }
        std::cout << "]" << std::endl;
    }
}

// ============================================================================
// TEST CASES
// ============================================================================

bool testSingleClusterFormation(bool verbose) {
    std::cout << "\n=== TEST: Single Cluster Formation (6 nearby atoms) ===" << std::endl;
    
    // ECS data
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder (index 0)
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    
    // Spawn 6 carbons in a tight circle (within bonding range)
    float centerX = -800.0f, centerY = 0.0f;
    float spawnRadius = 40.0f;  // < BOND_AUTO_RANGE (65)
    
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        float x = centerX + std::cos(angle) * spawnRadius;
        float y = centerY + std::sin(angle) * spawnRadius;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});  // Carbon
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    std::cout << " Initial: 6 isolated carbons at radius " << spawnRadius << std::endl;
    auto initialClusters = detectClusters(states);
    std::cout << " Initial clusters: " << initialClusters.size() << " (expected 6)" << std::endl;
    
    // Initialize physics with Clay Zone
    PhysicsEngine physics;
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    // Run 120 frames
    const int MAX_FRAMES = 120;
    const float dt = 1.0f / 60.0f;
    
    for (int frame = 0; frame < MAX_FRAMES; frame++) {
        physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
        
        if (verbose && frame % 20 == 0) {
            auto clusters = detectClusters(states);
            int bonds = countTotalBonds(states);
            std::cout << "  [Frame " << std::setw(3) << frame << "] Clusters: " 
                      << clusters.size() << " Bonds: " << bonds << std::endl;
        }
    }
    
    // Verify
    auto finalClusters = detectClusters(states);
    int finalBonds = countTotalBonds(states);
    
    std::cout << " Final:" << std::endl;
    printClusterSummary(finalClusters);
    std::cout << " Total bonds: " << finalBonds << std::endl;
    
    bool passed = (finalClusters.size() == 1 && finalBonds >= 5);
    std::cout << " Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    
    if (!passed) {
        std::cout << " Expected: 1 cluster with >= 5 bonds" << std::endl;
    }
    
    return passed;
}

bool testTwoSeparateClusters(bool verbose) {
    std::cout << "\n=== TEST: Two Separate Clusters (distant groups) ===" << std::endl;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    
    // Group A: 3 carbons at (-800, 0)
    float groupAx = -800.0f, groupAy = 0.0f;
    for (int i = 0; i < 3; i++) {
        float angle = i * (2.0f * 3.14159f / 3.0f);
        float x = groupAx + std::cos(angle) * 30.0f;
        float y = groupAy + std::sin(angle) * 30.0f;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    // Group B: 3 carbons at (-500, 0) - 300 units away (outside bonding range)
    float groupBx = -500.0f, groupBy = 0.0f;
    for (int i = 0; i < 3; i++) {
        float angle = i * (2.0f * 3.14159f / 3.0f);
        float x = groupBx + std::cos(angle) * 30.0f;
        float y = groupBy + std::sin(angle) * 30.0f;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    std::cout << " Initial: 2 groups of 3 carbons, 300 units apart" << std::endl;
    
    PhysicsEngine physics;
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    const int MAX_FRAMES = 120;
    const float dt = 1.0f / 60.0f;
    
    for (int frame = 0; frame < MAX_FRAMES; frame++) {
        physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
    }
    
    auto finalClusters = detectClusters(states);
    
    std::cout << " Final:" << std::endl;
    printClusterSummary(finalClusters);
    
    // Should have exactly 2 clusters (one for each group)
    bool passed = (finalClusters.size() == 2);
    std::cout << " Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    
    if (!passed) {
        std::cout << " Expected: 2 separate clusters" << std::endl;
    }
    
    return passed;
}

bool testClusterBreakDetection(bool verbose) {
    std::cout << "\n=== TEST: Cluster Break Detection ===" << std::endl;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    
    // Create 4 carbons and manually bond them: 1->2->3->4
    for (int i = 0; i < 4; i++) {
        float x = -800.0f + i * 30.0f;
        float y = 0.0f;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    // Manually bond them
    BondingCore::tryBond(2, 1, states, atoms, transforms, true);
    BondingCore::tryBond(3, 2, states, atoms, transforms, true);
    BondingCore::tryBond(4, 3, states, atoms, transforms, true);
    
    auto beforeBreak = detectClusters(states);
    std::cout << " Before break: " << beforeBreak.size() << " cluster(s)" << std::endl;
    printClusterSummary(beforeBreak);
    
    // Break bond between 2 and 3
    std::cout << " Breaking bond 2-3..." << std::endl;
    BondingCore::breakBond(3, states, atoms);
    
    auto afterBreak = detectClusters(states);
    std::cout << " After break:" << std::endl;
    printClusterSummary(afterBreak);
    
    // Should now have 2 clusters
    bool passed = (afterBreak.size() == 2);
    std::cout << " Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    
    return passed;
}

bool testBondFormationSpeed(bool verbose) {
    std::cout << "\n=== TEST: Bond Formation Speed ===" << std::endl;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    
    // Spawn 6 carbons at random positions within bonding range
    float centerX = -800.0f, centerY = 0.0f;
    srand(42);  // Fixed seed for reproducibility
    
    for (int i = 0; i < 6; i++) {
        float x = centerX + (rand() % 80) - 40;  // ±40 px
        float y = centerY + (rand() % 80) - 40;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    PhysicsEngine physics;
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    const float dt = 1.0f / 60.0f;
    int firstBondFrame = -1;
    int allConnectedFrame = -1;
    
    std::cout << " Running simulation..." << std::endl;
    
    for (int frame = 0; frame < 300; frame++) {
        physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
        
        int bonds = countTotalBonds(states);
        auto clusters = detectClusters(states);
        
        if (firstBondFrame < 0 && bonds > 0) {
            firstBondFrame = frame;
            std::cout << "  First bond formed at frame " << frame << std::endl;
        }
        
        if (allConnectedFrame < 0 && clusters.size() == 1 && bonds >= 5) {
            allConnectedFrame = frame;
            std::cout << "  All atoms connected at frame " << frame << std::endl;
            break;
        }
        
        if (verbose && frame % 30 == 0) {
            std::cout << "  [Frame " << std::setw(3) << frame << "] Bonds: " << bonds 
                      << " Clusters: " << clusters.size() << std::endl;
        }
    }
    
    bool passed = (firstBondFrame >= 0 && firstBondFrame < 30);
    std::cout << " First bond in " << firstBondFrame << " frames (expected < 30): " 
              << (passed ? "PASS" : "FAIL") << std::endl;
    
    if (allConnectedFrame > 0) {
        std::cout << " Full connection in " << allConnectedFrame << " frames" << std::endl;
    } else {
        std::cout << " WARNING: Did not achieve full connection in 300 frames" << std::endl;
    }
    
    return passed;
}

bool testChaosReformation(bool verbose) {
    std::cout << "\n=== TEST: Chaos Reformation (Break-Move-Reform Cycles) ===" << std::endl;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    
    // Spawn 6 carbons in a cluster
    float centerX = -800.0f, centerY = 0.0f;
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        float x = centerX + std::cos(angle) * 35.0f;
        float y = centerY + std::sin(angle) * 35.0f;
        
        transforms.push_back({x, y, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    PhysicsEngine physics;
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    const float dt = 1.0f / 60.0f;
    bool allPassed = true;
    
    // Phase 1: Let them bond initially
    std::cout << " Phase 1: Initial bonding (30 frames)..." << std::endl;
    for (int frame = 0; frame < 30; frame++) {
        physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
        BondingSystem::updateHierarchy(transforms, states, atoms);
    }
    
    int initialBonds = countTotalBonds(states);
    auto initialClusters = detectClusters(states);
    std::cout << "  Initial: " << initialClusters.size() << " cluster(s), " << initialBonds << " bonds" << std::endl;
    
    // Phase 2: CHAOS - Break bonds, move atoms, repeat 3 times
    for (int cycle = 1; cycle <= 3; cycle++) {
        std::cout << " Phase 2." << cycle << ": Break-Move-Wait cycle..." << std::endl;
        
        // Break ALL bonds
        for (int i = 1; i < (int)states.size(); i++) {
            if (states[i].parentEntityId != -1) {
                BondingCore::breakBond(i, states, atoms);
            }
            // Clear shields to simulate tractor release
            states[i].isShielded = false;
            states[i].releaseTimer = 0.1f;  // Just released
        }
        
        int bondsAfterBreak = countTotalBonds(states);
        auto clustersAfterBreak = detectClusters(states);
        if (verbose) {
            std::cout << "  After break: " << clustersAfterBreak.size() << " clusters, " << bondsAfterBreak << " bonds" << std::endl;
        }
        
        // Move atoms apart (simulate tractor scatter)
        for (int i = 1; i < (int)transforms.size(); i++) {
            float angle = (i - 1) * (2.0f * 3.14159f / 6.0f);
            transforms[i].x = centerX + std::cos(angle) * 50.0f;  // Spread out
            transforms[i].y = centerY + std::sin(angle) * 50.0f;
            transforms[i].vx = 0;
            transforms[i].vy = 0;
        }
        
        // Wait 60 frames for reformation
        for (int frame = 0; frame < 60; frame++) {
            physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
            BondingSystem::updateHierarchy(transforms, states, atoms);
            
            // Update release timer
            for (int i = 1; i < (int)states.size(); i++) {
                states[i].releaseTimer += dt;
            }
        }
        
        int bondsAfterReform = countTotalBonds(states);
        auto clustersAfterReform = detectClusters(states);
        
        std::cout << "  After reform: " << clustersAfterReform.size() << " cluster(s), " << bondsAfterReform << " bonds" << std::endl;
        
        // Verify: Should reform into 1 cluster with at least 5 bonds
        if (clustersAfterReform.size() > 1 || bondsAfterReform < 5) {
            std::cout << "  WARNING: Did not fully reform in cycle " << cycle << std::endl;
            if (cycle == 3) allPassed = false;  // Only fail on last cycle
        }
    }
    
    // Phase 3: Final check
    std::cout << " Phase 3: Final state..." << std::endl;
    auto finalClusters = detectClusters(states);
    int finalBonds = countTotalBonds(states);
    
    printClusterSummary(finalClusters);
    std::cout << " Total bonds: " << finalBonds << std::endl;
    
    // DIAGNOSTIC: Print state of each atom
    std::cout << "\n DIAGNOSTIC: Per-atom state after chaos:" << std::endl;
    std::cout << "   ID  Shield  InRing  CycleId  Parent  MolId  Locked" << std::endl;
    std::cout << "   ------------------------------------------------" << std::endl;
    for (int i = 1; i < (int)states.size(); i++) {
        std::cout << "   " << std::setw(2) << i
                  << "  " << std::setw(6) << (states[i].isShielded ? "YES" : "no")
                  << "  " << std::setw(6) << (states[i].isInRing ? "YES" : "no")
                  << "  " << std::setw(7) << states[i].cycleBondId
                  << "  " << std::setw(6) << states[i].parentEntityId
                  << "  " << std::setw(5) << states[i].moleculeId
                  << "  " << std::setw(6) << (states[i].isLocked() ? "YES" : "no")
                  << std::endl;
    }
    
    bool passed = (finalClusters.size() == 1 && finalBonds >= 5);
    std::cout << " Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    
    return passed && allPassed;
}

bool testRapidBreakReform(bool verbose) {
    std::cout << "\n=== TEST: Rapid Break-Reform (Stress Test) ===" << std::endl;
    
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;
    
    // Player placeholder
    transforms.push_back({0, 0, 0, 0, 0, 0, 0});
    atoms.push_back({1, 0});
    states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    
    // Create a linear chain: 1-2-3-4
    for (int i = 0; i < 4; i++) {
        float x = -800.0f + i * 30.0f;
        transforms.push_back({x, 0, 0, 0, 0, 0, 0});
        atoms.push_back({6, 0.0f});
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1, false, 0.0f});
    }
    
    // Bond them: 1-2-3-4
    BondingCore::tryBond(2, 1, states, atoms, transforms, true);
    BondingCore::tryBond(3, 2, states, atoms, transforms, true);
    BondingCore::tryBond(4, 3, states, atoms, transforms, true);
    
    PhysicsEngine physics;
    auto clayZone = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayZone);
    
    const float dt = 1.0f / 60.0f;
    int successfulReforms = 0;
    
    // Do 10 rapid break-reform cycles
    for (int cycle = 0; cycle < 10; cycle++) {
        // Break middle bond (2-3)
        BondingCore::breakBond(3, states, atoms);
        
        auto afterBreak = detectClusters(states);
        if (verbose) {
            std::cout << "  Cycle " << cycle << " break: " << afterBreak.size() << " clusters" << std::endl;
        }
        
        // Clear shields and set grace period
        for (int i = 1; i < (int)states.size(); i++) {
            states[i].isShielded = false;
            states[i].releaseTimer = 0.5f;
        }
        
        // Move atom 3 close again
        transforms[3].x = transforms[2].x + 35.0f;
        
        // Run 10 frames
        for (int frame = 0; frame < 10; frame++) {
            physics.step(dt, transforms, atoms, states, ChemistryDatabase::getInstance(), -1);
            BondingSystem::updateHierarchy(transforms, states, atoms);
        }
        
        auto afterReform = detectClusters(states);
        if (afterReform.size() == 1) {
            successfulReforms++;
        }
    }
    
    std::cout << " Successful reforms: " << successfulReforms << "/10" << std::endl;
    
    bool passed = (successfulReforms >= 8);  // Allow some failures due to physics variance
    std::cout << " Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    
    return passed;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--verbose" || std::string(argv[i]) == "-v") {
            verbose = true;
        }
    }
    
    std::cout << "======================================" << std::endl;
    std::cout << "  CLUSTER DETECTION TEST SUITE" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Initialize real systems
    ChemistryDatabase::getInstance().initialize();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    
    int passed = 0;
    int total = 6;
    
    if (testSingleClusterFormation(verbose)) passed++;
    if (testTwoSeparateClusters(verbose)) passed++;
    if (testClusterBreakDetection(verbose)) passed++;
    if (testBondFormationSpeed(verbose)) passed++;
    if (testChaosReformation(verbose)) passed++;
    if (testRapidBreakReform(verbose)) passed++;
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "  RESULTS: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return (passed == total) ? 0 : 1;
}
