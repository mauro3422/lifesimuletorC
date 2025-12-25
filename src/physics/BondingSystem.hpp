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
    // Intenta unir una entidad a otra en un slot libre
    static bool tryBond(int sourceId, int targetId, 
                       std::vector<StateComponent>& states,
                       const std::vector<AtomComponent>& atoms,
                       const std::vector<TransformComponent>& transforms);

    // Actualiza las posiciones de los átomos hijos basándose en la jerarquía
    static void updateHierarchy(std::vector<TransformComponent>& transforms,
                               std::vector<StateComponent>& states,
                               const std::vector<AtomComponent>& atoms);

    // Evolución molecular autónoma: Permite que los NPCs se unan espontáneamente
    static void updateSpontaneousBonding(std::vector<StateComponent>& states,
                                         const std::vector<AtomComponent>& atoms,
                                         const std::vector<TransformComponent>& transforms);

    // Retorna el primer índice de slot libre (usado para magnetic docking)
    static int getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states,
                                const std::vector<AtomComponent>& atoms);

private:
    // Retorna el índice del slot libre más cercano a una posición relativa
    static int getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms);
};

#endif
