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

        // 2. Four Carbon atoms in clay zone (spread out, will need to be moved by player)
        // Clay zone is at x:100-300, y:100-300 (configured in EnvironmentManager)
        float clayX = 200.0f;
        float clayY = 200.0f;
        
        // Carbon 1
        transforms.push_back({ clayX - 30.0f, clayY - 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({6, 0.0f}); // Carbon
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1});
        
        // Carbon 2
        transforms.push_back({ clayX + 30.0f, clayY - 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({6, 0.0f}); // Carbon
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1});
        
        // Carbon 3
        transforms.push_back({ clayX + 30.0f, clayY + 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({6, 0.0f}); // Carbon
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1, -1});
        
        // Carbon 4
        transforms.push_back({ clayX - 30.0f, clayY + 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({6, 0.0f}); // Carbon
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1, false, 0, -1});
        
        TraceLog(LOG_INFO, "[World] TEST MODE - Created 4 Carbons at clay zone (%.0f, %.0f)", clayX, clayY);
        TraceLog(LOG_INFO, "[World] TEST MODE - Carbon IDs: 1, 2, 3, 4");
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
