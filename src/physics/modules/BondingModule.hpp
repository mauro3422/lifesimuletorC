#ifndef BONDING_MODULE_HPP
#define BONDING_MODULE_HPP

#include "../../ecs/components.hpp"
#include <vector>

/**
 * Módulo especializado en la formación y ruptura de enlaces químicos.
 */
class BondingModule {
public:
    // Evalúa si dos átomos están lo suficientemente cerca para enlazarse
    static bool canBond(const AtomComponent& a, const AtomComponent& b, float distance);
    
    // Aplica fuerzas de muelle (Spring forces) entre dos átomos enlazados
    static void applyBondForce(TransformComponent& a, TransformComponent& b, float targetDist);
};

#endif
