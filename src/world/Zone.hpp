#ifndef ZONE_HPP
#define ZONE_HPP

#include "raylib.h"
#include <string>
#include "../ecs/components.hpp"

/**
 * Base class for all environmental zones (Islands).
 * Each zone defines its own physical and chemical modifiers.
 */
class Zone {
public:
    Zone(std::string name, Rectangle bounds, Color color) 
        : name(name), bounds(bounds), color(color) {}
    virtual ~Zone() = default;

    virtual void apply(TransformComponent& transform, StateComponent& state, float dt) = 0;
    virtual void draw() {
        DrawRectangleRec(bounds, Fade(color, 0.1f));
        DrawRectangleLinesEx(bounds, 1, Fade(color, 0.3f));
        DrawText(name.c_str(), (int)bounds.x + 5, (int)bounds.y + 5, 10, Fade(color, 0.5f));
    }

    virtual float getBondRangeMultiplier() const { return 1.0f; }
    virtual float getBondAngleMultiplier() const { return 1.0f; }

    bool contains(Vector2 pos) const {
        return CheckCollisionPointRec(pos, bounds);
    }

    std::string getName() const { return name; }
    Rectangle getBounds() const { return bounds; }

protected:
    std::string name;
    Rectangle bounds;
    Color color;
};

#endif // ZONE_HPP
