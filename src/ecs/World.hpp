#ifndef WORLD_HPP
#define WORLD_HPP

#include <vector>
#include "components.hpp"
#include "../core/Config.hpp"
#include "raylib.h"

/**
 * World: Contenedor central para el ECS.
 * Encapsula los vectores de componentes y la lógica de inicialización.
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

        // 1. JUGADOR (Siempre ID 0)
        transforms.push_back({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        atoms.push_back({1, 0.0f}); // Hidrógeno inicial
        states.push_back({false, -1, -1, -1, 1.0f}); // dockingProgress = 1.0 (no animacion)
        TraceLog(LOG_INFO, "[World] Jugador inicializado en (0,0)");

        // 2. MUNDO
        for (int i = 1; i < Config::INITIAL_ATOM_COUNT; i++) {
            int atomicNum = Config::ATOM_TYPES[GetRandomValue(0, Config::ATOM_TYPES_COUNT - 1)];
            
            // New spawn spread logic: Muy denso alrededor del origen (0,0)
            TransformComponent tr = {
                (float)GetRandomValue(-250, 250), 
                (float)GetRandomValue(-250, 250), 
                (float)GetRandomValue(-40, 40),
                (float)GetRandomValue(-100, 100) / 100.0f * Config::INITIAL_VEL_RANGE,
                (float)GetRandomValue(-100, 100) / 100.0f * Config::INITIAL_VEL_RANGE,
                (float)GetRandomValue(-100, 100) / 100.0f * Config::INITIAL_VEL_RANGE,
                0.0f // rotation
            };

            transforms.push_back(tr);
            atoms.push_back({atomicNum, 0.0f});
            states.push_back({false, -1, -1, -1, 1.0f}); // dockingProgress = 1.0 (no animacion)
        }
    }

    size_t getEntityCount() const { return atoms.size(); }
    
    /**
     * Retorna los índices de todos los átomos que pertenecen a la misma molécula.
     */
    std::vector<int> getMoleculeMembers(int entityId) const {
        std::vector<int> members;
        if (entityId < 0 || entityId >= (int)states.size()) return members;

        // Identificamos la raíz de la molécula
        int rootId = (states[entityId].moleculeId == -1) ? entityId : states[entityId].moleculeId;

        for (int i = 0; i < (int)states.size(); i++) {
            if (states[i].moleculeId == rootId || i == rootId) {
                members.push_back(i);
            }
        }
        return members;
    }
};

#endif
