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
        states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1}); // childCount=0, occupiedSlots=0, cycleBondId=-1
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
            states.push_back({false, -1, -1, -1, 1.0f, false, 0, 0, -1}); 
        }
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
