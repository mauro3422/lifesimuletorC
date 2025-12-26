#ifndef CLAY_ZONE_HPP
#define CLAY_ZONE_HPP

#include "../Zone.hpp"
#include "../../core/MathUtils.hpp"
#include "../../core/Config.hpp"
#include <cmath>

/**
 * The Clay Island: A catalytic zone that attracts and stabilizes molecules.
 */
class ClayZone : public Zone {
public:
    ClayZone(Rectangle bounds) 
        : Zone("CLAY ISLAND", bounds, BROWN) {}

    void apply(TransformComponent& transform, StateComponent& state, float dt) override {
        // 1. ADSORPTION (Weak force toward center)
        Vector2 center = { bounds.x + bounds.width/2.0f, bounds.y + bounds.height/2.0f };
        float dx = center.x - transform.x;
        float dy = center.y - transform.y;
        float dist = std::sqrt(dx*dx + dy*dy);
        
        if (dist > 1.0f) {
            float force = 5.0f; // Low adsorption force
            transform.vx += (dx / dist) * force * dt;
            transform.vy += (dy / dist) * force * dt;
        }

        // 2. INCREASED DRAG (Stickiness)
        transform.vx *= 0.95f; 
        transform.vy *= 0.95f;

        // 3. THERMODYNAMIC AGITATION (Local heat)
        transform.vx += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER * 2.0f;
        transform.vy += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER * 2.0f;
        
        // Note: Bonding probability boost is handled in BondingSystem by checking if inside ClayZone
    }

    float getBondRangeMultiplier() const override { return 1.5f; } // Facilitates long distance bonding
    float getBondAngleMultiplier() const override { return 1.2f; } // Relaxed geometry for catalysis
};

#endif // CLAY_ZONE_HPP
