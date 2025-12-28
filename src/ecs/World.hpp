#ifndef WORLD_HPP
#define WORLD_HPP

#include <vector>
#include "components.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../core/MathUtils.hpp"
#include "raylib.h"

/**
 * World: Central container for the ECS.
 * Encapsulates component vectors and initialization logic.
 */
class World {
public:
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;

    void initialize() {
        transforms.clear();
        atoms.clear();
        states.clear();

        // 1. PLAYER (Always ID 0)
        transforms.push_back({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({1, 0.0f}); // Initial Hydrogen
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1}); // ringSize=0, ringInstanceId=-1
        TraceLog(LOG_INFO, "[World] Player initialized at (0,0)");

        // 2. WORLD POPULATION
        ChemistryDatabase& db = ChemistryDatabase::getInstance();
        for (int i = 1; i < Config::INITIAL_ATOM_COUNT; i++) {
            int atomicNum = db.getRandomSpawnableAtomicNumber();
            
            int rangeXY = (int)Config::SPAWN_RANGE_XY;
            int rangeZ = (int)Config::SPAWN_RANGE_Z;
            
            TransformComponent tr = {
                (float)GetRandomValue(-rangeXY, rangeXY), 
                (float)GetRandomValue(-rangeXY, rangeXY), 
                (float)GetRandomValue(-rangeZ, rangeZ),
                (float)GetRandomValue(-100, 100) / Config::SPAWN_VEL_DIVISOR * Config::INITIAL_VEL_RANGE,
                (float)GetRandomValue(-100, 100) / Config::SPAWN_VEL_DIVISOR * Config::INITIAL_VEL_RANGE,
                (float)GetRandomValue(-100, 100) / Config::SPAWN_VEL_DIVISOR * Config::INITIAL_VEL_RANGE,
                0.0f // rotation
            };

            transforms.push_back(tr);
            atoms.push_back({atomicNum, 0.0f});
            states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1}); 
        }
    }

    /**
     * TEST MODE: Minimal world for debugging ring formation
     * Creates only 4 carbons in a clay zone (x:100-200, y:100-200)
     */
    void initializeTestMode() {
        transforms.clear();
        atoms.clear();
        states.clear();

        // 1. PLAYER (Always ID 0)
        transforms.push_back({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({1, 0.0f}); // Hydrogen
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1});
        TraceLog(LOG_INFO, "[World] TEST MODE - Player initialized at (0,0)");

        // 2. Eight Carbon atoms in "Ladder" Formation (Stacked Squares)
        // Group 1 (Bottom Square)
        float g1X = -1200.0f;
        float g1Y = -350.0f; // Lower
        transforms.push_back({ g1X - 25, g1Y - 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        transforms.push_back({ g1X + 25, g1Y - 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        transforms.push_back({ g1X + 25, g1Y + 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        transforms.push_back({ g1X - 25, g1Y + 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});

        // Group 2 (Top Square) - Placed 60 units above (Y axis)
        float g2X = -1200.0f;
        float g2Y = -410.0f; // Higher (Negative Y is up?) Raylib Y+ is down. So -410 is "Above" physically on screen?
        transforms.push_back({ g2X - 25, g2Y - 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        transforms.push_back({ g2X + 25, g2Y - 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        transforms.push_back({ g2X + 25, g2Y + 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        transforms.push_back({ g2X - 25, g2Y + 25, 0, 0, 0, 0, 0 }); atoms.push_back({6, 0.0f}); 
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, 0, -1, false});
        
        TraceLog(LOG_INFO, "[World] TEST MODE - Created Stacked Squares at Clay Zone");
    }

    size_t getEntityCount() const { return atoms.size(); }
    
    /**
     * Returns the indices of all atoms belonging to the same molecule.
     */
    std::vector<int> getMoleculeMembers(int entityId) const {
        return MathUtils::getMoleculeMembers(entityId, states);
    }
};

#endif
