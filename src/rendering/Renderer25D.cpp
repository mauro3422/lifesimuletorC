#include "Renderer25D.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include <algorithm>
#include <cmath>

void Renderer25D::drawAtoms(const std::vector<TransformComponent>& transforms, const std::vector<AtomComponent>& atoms, const std::vector<StateComponent>& states) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    
    // 1. DRAW BONDS FIRST (Rendered behind atoms)
    for (int i = 0; i < (int)states.size(); i++) {
        const StateComponent& state = states[i];
        if (state.isClustered && state.parentEntityId != -1) {
            int pId = state.parentEntityId;
            const TransformComponent& trChild = transforms[i];
            const TransformComponent& trParent = transforms[pId];
            
            float dist = MathUtils::dist(trChild.x, trChild.y, trParent.x, trParent.y);
            
            // Skip drawing if atoms are too close (degenerate) OR too far (broken state)
            if (dist < 0.01f) continue;
            if (dist > Config::MAX_BOND_RENDER_DIST) continue; // Hide overly stretched bonds
            
            Vector2 dir = MathUtils::normalize(Vector2{trChild.x - trParent.x, trChild.y - trParent.y});
            float dirX = dir.x;
            float dirY = dir.y;
            
            const Element& parentEl = db.getElement(atoms[pId].atomicNumber);
            const Element& childEl = db.getElement(atoms[i].atomicNumber);
            float parentRadius = parentEl.vdWRadius * Config::BASE_ATOM_RADIUS;
            float childRadius = childEl.vdWRadius * Config::BASE_ATOM_RADIUS;
            
            Vector2 start = { trParent.x + dirX * parentRadius, trParent.y + dirY * parentRadius };
            Vector2 end = { trChild.x - dirX * childRadius, trChild.y - dirY * childRadius };
            
            float scale = 1.0f + (((trChild.z + trParent.z) / 2.0f) * Config::DEPTH_SCALE_FACTOR);
            if (scale < Config::RENDER_MIN_SCALE) scale = Config::RENDER_MIN_SCALE;

            Color bondColor = { 
                (unsigned char)((parentEl.color.r + childEl.color.r) / 2),
                (unsigned char)((parentEl.color.g + childEl.color.g) / 2),
                (unsigned char)((parentEl.color.b + childEl.color.b) / 2),
                255 
            };
            
            DrawLineEx(start, end, Config::RENDER_BOND_THICKNESS_BG * scale, BLACK);
            DrawLineEx(start, end, Config::RENDER_BOND_THICKNESS_FG * scale, bondColor);
        }
    }

    // 2. DRAW ATOMS (rendered on top of bonds)
    static std::vector<int> indices;
    if (indices.size() != transforms.size()) {
        indices.resize(transforms.size());
        for (int i = 0; i < (int)indices.size(); i++) indices[i] = i;
    }

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return transforms[a].z < transforms[b].z;
    });

    for (int idx : indices) {
        const TransformComponent& tr = transforms[idx];
        const Element& element = db.getElement(atoms[idx].atomicNumber);
        
        float scale = 1.0f + (tr.z * Config::DEPTH_SCALE_FACTOR); 
        if (scale < Config::RENDER_MIN_SCALE) scale = Config::RENDER_MIN_SCALE;

        float radius = (element.vdWRadius * Config::BASE_ATOM_RADIUS) * scale;
        
        int bVal = std::clamp(Config::COLOR_BRIGHTNESS_BASE + (int)tr.z, Config::MIN_BRIGHTNESS, 255);
        unsigned char brightness = (unsigned char)bVal;

        Color c = element.color;
        Color finalColor = { 
            (unsigned char)(c.r * brightness / 255),
            (unsigned char)(c.g * brightness / 255),
            (unsigned char)(c.b * brightness / 255),
            255 
        };

        DrawCircleGradient((int)tr.x, (int)tr.y, radius, finalColor, BLACK);
    }
}
