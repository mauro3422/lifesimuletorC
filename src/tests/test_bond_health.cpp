#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>

#include "raylib.h"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"
#include "physics/PhysicsEngine.hpp"
#include "physics/BondingSystem.hpp"
#include "physics/SpatialGrid.hpp"
#include "chemistry/ChemistryDatabase.hpp"
#include "chemistry/StructureRegistry.hpp"

#include <fstream>
#include "world/zones/ClayZone.hpp"

// Global ECS (Simplified simulation)
std::vector<TransformComponent> transforms;
std::vector<AtomComponent> atoms;
std::vector<StateComponent> states;
SpatialGrid grid(50.0f);
std::ofstream diagLog;

void logAtomState(int i, const std::string& prefix = "") {
    auto& s = states[i];
    auto& t = transforms[i];
    
    std::string msg = prefix + " [Atom " + std::to_string(i) + "] Pos:(" + std::to_string(t.x) + "," + std::to_string(t.y) + 
                      ") Mol:" + std::to_string(s.moleculeId) + 
                      " Shield:" + (s.isShielded ? "YES" : "NO") + 
                      " Clust:" + (s.isClustered ? "YES" : "NO") + 
                      " Parent:" + std::to_string(s.parentEntityId) + 
                      " Slots:" + std::to_string(s.occupiedSlots) + 
                      " Ring:" + (s.isInRing ? "YES" : "NO");
    
    std::cout << msg << std::endl;
    if (diagLog.is_open()) diagLog << msg << std::endl;
}

int main(int argc, char* argv[]) {
    diagLog.open("bond_health_diag.log");
    // 1. Initialize Systems
    InitWindow(100, 100, "Bond Health Test");
    SetTargetFPS(60);
    SetTraceLogLevel(LOG_INFO); // Show my traces!

    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    StructureRegistry::getInstance().loadFromDisk("data/structures.json");
    PhysicsEngine physics;

    // Add a ClayZone to the environment for ring formation
    auto zone = std::make_shared<ClayZone>((Rectangle){-1000, -1000, 2000, 2000});
    physics.getEnvironment().addZone(zone);

    const float centerX = 0.0f;
    const float centerY = 0.0f;
    const float bondDist = Config::BOND_IDEAL_DIST;

    // 2. Setup Hexagon Atoms
    std::cout << "\n=== SETUP: Spawning Hexagon ===" << std::endl;
    if (diagLog.is_open()) diagLog << "\n=== SETUP: Spawning Hexagon ===" << std::endl;
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * 3.14159f / 6.0f);
        
        TransformComponent t = {0};
        t.x = centerX + cos(angle) * bondDist;
        t.y = centerY + sin(angle) * bondDist;
        transforms.push_back(t);

        AtomComponent a = {0};
        a.atomicNumber = 6;
        atoms.push_back(a);

        StateComponent s;
        s.moleculeId = i;
        states.push_back(s);
    }
    grid.update(transforms);

    // 3. Stabilization Phase (Let the hexagon form)
    std::cout << "Stabilizing hexagon for 60 frames..." << std::endl;
    for (int f = 0; f < 60; f++) {
        physics.step(Config::FIXED_DELTA_TIME, transforms, atoms, states, db, -1);
        grid.update(transforms);
    }

    // Verify ring formation
    int ringAtoms = 0;
    for (auto& s : states) if (s.isInRing) ringAtoms++;
    std::cout << "Hexagon formed: " << ringAtoms << "/6 atoms in ring." << std::endl;

    // 4. PICKUP PHASE (Simulate TractorBeam capturing atom 3)
    int targetIdx = 3;
    std::cout << "\n=== ACTION: Capturing Atom " << targetIdx << " ===" << std::endl;
    
    // Logic from Player::applyPhysics + BondingSystem::breakAllBonds
    BondingSystem::breakAllBonds(targetIdx, states, atoms);
    states[targetIdx].isShielded = true;
    
    // Move it slightly inside the ring (crowded area)
    transforms[targetIdx].x = centerX + 5.0f;
    transforms[targetIdx].y = centerY + 5.0f;
    grid.update(transforms);

    std::cout << "State after Capture:" << std::endl;
    logAtomState(targetIdx);
    for(int i=0; i<6; i++) {
        if(states[i].parentEntityId == targetIdx || states[targetIdx].parentEntityId == i) {
             std::cout << "  [ERROR] Atom " << targetIdx << " still bonded to " << i << "!" << std::endl;
        }
    }

    // 5. WAIT PHASE (Held in shield)
    std::cout << "\nHolding shielded for 10 frames..." << std::endl;
    for (int f = 0; f < 10; f++) {
        physics.step(Config::FIXED_DELTA_TIME, transforms, atoms, states, db, targetIdx);
        grid.update(transforms);
    }

    // 6. RELEASE PHASE
    std::cout << "\n=== ACTION: Releasing Atom " << targetIdx << " ===" << std::endl;
    states[targetIdx].isShielded = false;
    // Force a grid update (one of the suspected fixes)
    grid.update(transforms);

    // 7. MONITORING PHASE
    std::cout << "Monitoring for Re-bonding (max 100 frames)..." << std::endl;
    bool bonded = false;
    for (int f = 0; f < 100; f++) {
        physics.step(Config::FIXED_DELTA_TIME, transforms, atoms, states, db, -1);
        grid.update(transforms);

        if (states[targetIdx].isClustered) {
            std::cout << "  [SUCCESS] Atom " << targetIdx << " rebonded at frame " << f << " to Parent " << states[targetIdx].parentEntityId << std::endl;
            bonded = true;
            break;
        }

        if (f % 10 == 0) {
            logAtomState(targetIdx, "  Frame " + std::to_string(f) + ":");
        }
    }

    if (!bonded) {
        std::cout << "\n  [FAILURE] Atom " << targetIdx << " never rebonded!" << std::endl;
        std::cout << "Final Diagnostics:" << std::endl;
        for(int i=0; i<6; i++) {
            logAtomState(i);
            // Check why others might reject targetIdx
            int root = MathUtils::findMoleculeRoot(i, states);
            std::cout << "    isShielded: " << (states[root].isShielded ? "YES" : "NO") << " | maxBonds logic check..." << std::endl;
        }
    }

    CloseWindow();
    return bonded ? 0 : 1;
}
