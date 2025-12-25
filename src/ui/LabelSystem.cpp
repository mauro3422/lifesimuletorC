#include "LabelSystem.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../core/Config.hpp"
#include <algorithm>

void LabelSystem::draw(const Camera2D& camera, 
                       const std::vector<TransformComponent>& transforms, 
                       const std::vector<AtomComponent>& atoms) {
    
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    float zoom = camera.zoom;
    const float ATOM_THRESHOLD = Config::LABEL_ATOM_THRESHOLD;
    
    for (size_t i = 0; i < transforms.size(); i++) {
        const TransformComponent& tr = transforms[i];
        const AtomComponent& atom = atoms[i];
        
        if (zoom >= ATOM_THRESHOLD) {
            const Element& element = db.getElement(atom.atomicNumber);
            
            float alpha = std::clamp((zoom - ATOM_THRESHOLD) * Config::LABEL_FADE_SPEED, 0.0f, 1.0f);
            if (alpha <= 0.05f) continue;

            Color textColor = Fade(WHITE, alpha);
            int fontSize = Config::LABEL_FONT_SIZE;
            int textX = (int)tr.x - (MeasureText(element.symbol.c_str(), fontSize) / 2);
            int textY = (int)tr.y - (fontSize / 2);

            DrawText(element.symbol.c_str(), textX, textY, fontSize, textColor);
        }
        else {
            if (i % 15 == 0) { 
                float alpha = std::clamp((ATOM_THRESHOLD - zoom) * Config::LABEL_FADE_SPEED, 0.0f, 0.8f);
                if (alpha <= 0.05f) continue;

                const char* molName = "Complex Cluster";
                int fontSize = Config::LABEL_FONT_SIZE + 2;
                int textX = (int)tr.x - (MeasureText(molName, fontSize) / 2);
                int textY = (int)tr.y - (fontSize / 2);

                DrawText(molName, textX, textY, fontSize, Fade(SKYBLUE, alpha));
            }
        }
    }
}
