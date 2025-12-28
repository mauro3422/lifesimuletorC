#ifndef ENVIRONMENT_MANAGER_HPP
#define ENVIRONMENT_MANAGER_HPP

#include "Zone.hpp"
#include <vector>
#include <memory>

/**
 * Manages all active environmental zones and orchestrates their influence.
 */
class EnvironmentManager {
public:
    void addZone(std::shared_ptr<Zone> zone) {
        zones.push_back(zone);
    }

    void update(std::vector<TransformComponent>& transforms, std::vector<StateComponent>& states, float dt) {
        for (size_t i = 0; i < transforms.size(); ++i) {
            Vector2 pos = { transforms[i].x, transforms[i].y };
            for (auto& zone : zones) {
                if (zone->contains(pos)) {
                    zone->apply(transforms[i], states[i], dt);
                }
            }
        }
    }

    float getBondRangeMultiplier(Vector2 pos) const {
        for (auto const& zone : zones) {
            if (zone->contains(pos)) return zone->getBondRangeMultiplier();
        }
        return 1.0f;
    }

    float getBondAngleMultiplier(Vector2 pos) const {
        for (auto const& zone : zones) {
            if (zone->contains(pos)) return zone->getBondAngleMultiplier();
        }
        return 1.0f;
    }

    // Check if position is in a zone that allows ring formation (e.g., Clay Zone)
    bool isInRingFormingZone(Vector2 pos) const {
        for (auto const& zone : zones) {
            if (zone->contains(pos) && zone->allowsRingFormation()) return true;
        }
        return false;
    }

    void draw() {
        for (auto& zone : zones) {
            zone->draw();
        }
    }

private:
    std::vector<std::shared_ptr<Zone>> zones;
};

#endif // ENVIRONMENT_MANAGER_HPP
