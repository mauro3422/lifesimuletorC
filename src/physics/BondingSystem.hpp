#ifndef BONDING_SYSTEM_HPP
#define BONDING_SYSTEM_HPP

#include "../ecs/components.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include <vector>

/**
 * SISTEMA DE ENLACES (Deterministic)
 * Maneja la unión de átomos en slots VSEPR y la actualización de moléculas rígidas.
 */
class BondingSystem {
public:
    enum BondError {
        SUCCESS = 0,
        VALENCY_FULL,
        DISTANCE_TOO_FAR,
        ANGLE_INCOMPATIBLE,
        ALREADY_CLUSTERED,
        INTERNAL_ERROR
    };

    // Intenta unir una entidad a otra en un slot libre
    // Si forced es true, ignora el umbral de ángulo (usado por el jugador)
    static BondError tryBond(int sourceId, int targetId, 
                       std::vector<StateComponent>& states,
                       const std::vector<AtomComponent>& atoms,
                       const std::vector<TransformComponent>& transforms,
                       bool forced = false);

    // Actualiza las posiciones de los átomos hijos basándose en la jerarquía
    static void updateHierarchy(std::vector<TransformComponent>& transforms,
                               std::vector<StateComponent>& states,
                               const std::vector<AtomComponent>& atoms);

    // Evolución molecular autónoma: Permite que los NPCs se unan espontáneamente
    // tractedEntityId permite excluir moléculas que están siendo manipuladas por el jugador
    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         const std::vector<AtomComponent>& atoms,
                                         const std::vector<TransformComponent>& transforms,
                                         int tractedEntityId = -1);

    // Retorna el primer índice de slot libre (usado para magnetic docking)
    static int getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states,
                                const std::vector<AtomComponent>& atoms);

private:
    // Retorna el índice del slot libre más cercano a una posición relativa
    static int getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle = false);
};

#endif
