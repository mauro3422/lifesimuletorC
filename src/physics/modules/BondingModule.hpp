#ifndef BONDING_MODULE_HPP
#define BONDING_MODULE_HPP

#include "../../ecs/components.hpp"
#include <vector>

/**
 * Specialized module for the formation and breakage of chemical bonds.
 */
class BondingModule {
public:
    // Evaluates if two atoms are close enough to form a bond
    static bool canBond(const AtomComponent& a, const AtomComponent& b, float distance);
    
    // Applies spring forces between two bonded atoms
    static void applyBondForce(TransformComponent& a, TransformComponent& b, float targetDist);
};

#endif
