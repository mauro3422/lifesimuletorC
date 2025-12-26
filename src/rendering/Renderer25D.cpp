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

    // --- PHASE 18: DRAW CYCLE BONDS (Membrane loops) ---
    for (int i = 0; i < (int)states.size(); i++) {
        const StateComponent& state = states[i];
        if (state.cycleBondId != -1) {
             // Only draw if i < cycleBondId to avoid double drawing
             if (i > state.cycleBondId) continue;

             int partnerId = state.cycleBondId;
             const TransformComponent& trA = transforms[i];
             const TransformComponent& trB = transforms[partnerId];
             
             float dist = MathUtils::dist(trA.x, trA.y, trB.x, trB.y);
             if (dist < 0.01f || dist > Config::MAX_BOND_RENDER_DIST) continue;

             Vector2 dir = MathUtils::normalize(Vector2{trA.x - trB.x, trA.y - trB.y});
             float dirX = dir.x;
             float dirY = dir.y;

             const Element& elA = db.getElement(atoms[i].atomicNumber);
             const Element& elB = db.getElement(atoms[partnerId].atomicNumber);
             float rA = elA.vdWRadius * Config::BASE_ATOM_RADIUS;
             float rB = elB.vdWRadius * Config::BASE_ATOM_RADIUS;

             Vector2 start = { trB.x + dirX * rB, trB.y + dirY * rB };
             Vector2 end = { trA.x - dirX * rA, trA.y - dirY * rA };
             
             float scale = 1.0f + (((trA.z + trB.z) / 2.0f) * Config::DEPTH_SCALE_FACTOR);
             if (scale < Config::RENDER_MIN_SCALE) scale = Config::RENDER_MIN_SCALE;

             // MEMBRANE VISUALIZATION: "Border" Style
             // Thicker, brighter, and solid opacity to act as a clear perimeter.
             Color bondColor = SKYBLUE; 
             
             DrawLineEx(start, end, Config::RENDER_BOND_THICKNESS_BG * 2.0f * scale, BLACK);
             DrawLineEx(start, end, Config::RENDER_BOND_THICKNESS_FG * 2.0f * scale, bondColor);
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

void Renderer25D::drawDebugSlots(int atomId, 
                               const std::vector<TransformComponent>& transforms, 
                               const std::vector<AtomComponent>& atoms) {
    if (atomId < 0 || atomId >= (int)atoms.size()) return;

    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& el = db.getElement(atoms[atomId].atomicNumber);
    const TransformComponent& tr = transforms[atomId];

    // Draw lines for each available slot
    // NOTE: This visualizes the EXACT vectors the Physics Engine uses (lines 73-78)
    for (const Vector3& slot : el.bondingSlots) {
        float targetX = tr.x + slot.x * Config::BOND_IDEAL_DIST;
        float targetY = tr.y + slot.y * Config::BOND_IDEAL_DIST;
        
        // Draw dashed line or simple line to target
        // Color Yellow to indicate "Potential Bond"
        DrawLineEx({tr.x, tr.y}, {targetX, targetY}, 2.0f, Fade(YELLOW, 0.6f));
        DrawCircle((int)targetX, (int)targetY, 3.0f, YELLOW);
    }
}
