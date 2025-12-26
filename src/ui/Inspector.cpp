#include "Inspector.hpp"
#include "UIWidgets.hpp"
#include "UIConfig.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../gameplay/DiscoveryLog.hpp"
#include "../gameplay/MissionManager.hpp"
#include "../core/LocalizationManager.hpp"
#include <string>

Inspector::Inspector() {}

void Inspector::draw(const Element& element, int entityID, InputHandler& input, std::vector<StateComponent>& states, std::vector<AtomComponent>& atoms) {
    int screenH = GetScreenHeight();
    
    // LAYOUT CONFIGURATION
    float margin = (float)Config::INSPECTOR_MARGIN;
    float width = UIConfig::INSPECTOR_WIDTH;
    float x = margin;
    float innerWidth = width - (UIConfig::INNER_PADDING * 2.0f);

    Color activeColor = currentMolecule ? currentMolecule->color : element.color;
    // If it's a cluster but NOT a recognized molecule, use a neutral color or the detected element's color
    if (!currentMolecule && states[entityID].parentEntityId != -1) activeColor = element.color;
    
    // --- STEP 1: CALCULATE ESTIMATED HEIGHT ---
    float calculatedHeight = 40.0f; // Base margin (header + padding)
    
    if (currentMolecule) {
        calculatedHeight = 260.0f; // Molecules are usually fixed but long
    } else {
        int totalAtoms = 0;
        for (auto const& [num, count] : currentComposition) totalAtoms += count;
        
        if (totalAtoms > 1) {
            // Cluster: Header + Status + Sep + CompH + (N * 15) + Sep + ObsH + Obs + Footer
            calculatedHeight = UIConfig::HEADER_HEIGHT + 15 + 8 + 15 + (currentComposition.size() * UIConfig::LIST_ITEM_HEIGHT) + 20 + 12 + 50 + 40;
        } else {
            calculatedHeight = (float)Config::INSPECTOR_HEIGHT;
        }
    }

    Rectangle rect = { x, (float)(screenH - calculatedHeight - margin), width, calculatedHeight };

    // 1. Premium Background with Glow
    UIWidgets::drawPanel(rect, input, activeColor);
    
    std::string headerTitle = currentMolecule ? TextFormat("[M] %s", currentMolecule->name.c_str()) : TextFormat("[+] %s", element.name.c_str());
    
    // If not a molecule but has > 1 atom, it's a transitory cluster
    int totalAtoms = 0;
    for (auto const& [num, count] : currentComposition) totalAtoms += count;

    if (!currentMolecule && totalAtoms > 1) {
        headerTitle = TextFormat("[C] %s", element.symbol.c_str());
    }
    
    UIWidgets::drawHeader(rect, headerTitle.c_str(), activeColor);

    // If in cluster mode (detected by having composition > 1 atom)
    // or if we specifically have a recognized molecule.
    if (currentMolecule) {
        drawMoleculeOverlay(rect, input);
        return; 
    } else if (totalAtoms > 1) {
        drawTransitoryMoleculeOverlay(rect, input);
        return;
    }

    float curX = rect.x + UIConfig::INNER_PADDING;
    float curY = rect.y + UIConfig::HEADER_HEIGHT + UIConfig::SPACING_MEDIUM + 5.0f; // Extra padding below header

    // 2. Element Card and Valencies (No redundant ID line)
    UIWidgets::drawElementCard(element, curX, curY, UIConfig::INSPECTOR_CARD_SIZE, input);
    
    int infoX = (int)curX + (int)UIConfig::INSPECTOR_CARD_SIZE + 7;
    int infoY = (int)curY + 2;
    DrawText(element.name.c_str(), infoX, infoY, UIConfig::FONT_SIZE_HEADER, WHITE);
    DrawText(TextFormat("[%s] #%d", element.symbol.c_str(), element.atomicNumber), infoX, infoY + 15, UIConfig::FONT_SIZE_LABEL, LIGHTGRAY);
    
    // Dynamic Valency Bar
    UIWidgets::drawProgressBar({ (float)infoX, (float)infoY + UIConfig::CARD_INFO_OFFSET_Y, 60, UIConfig::INSPECTOR_BAR_HEIGHT }, 0.0f, element.color, TextFormat("0/%d", element.maxBonds));

    curY += UIConfig::INSPECTOR_CARD_SIZE + UIConfig::SPACING_MEDIUM;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += UIConfig::SPACING_SMALL;

    // 4. Technical Data (Automatically update curY)
    auto& lm = LocalizationManager::getInstance();
    UIWidgets::drawValueLabel(lm.get("ui.inspector.electronegativity_short").c_str(), TextFormat("%.2f", element.electronegativity), curX, curY, innerWidth);
    UIWidgets::drawValueLabel(lm.get("ui.inspector.vdw_radius_short").c_str(), TextFormat("%d pm", (int)element.vdWRadius), curX, curY, innerWidth);
    UIWidgets::drawValueLabel(lm.get("ui.inspector.atomic_mass_short").c_str(), TextFormat("%.1f u", element.atomicMass), curX, curY, innerWidth);
    UIWidgets::drawValueLabel(lm.get("ui.inspector.max_bonds_short").c_str(), TextFormat("%d", element.maxBonds), curX, curY, innerWidth);
    
    // ORIGIN uses drawTextWrapped to prevent overlap
    DrawText(LocalizationManager::getInstance().get("ui.inspector.origin").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, Config::THEME_TEXT_SECONDARY);
    curY += UIConfig::SPACING_MEDIUM;
    UIWidgets::drawTextWrapped(element.origin.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, ColorAlpha(SKYBLUE, 0.8f));

    curY += UIConfig::SPACING_SMALL - 2.0f;
    DrawText(LocalizationManager::getInstance().get("ui.inspector.lore").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, SKYBLUE);
    curY += UIConfig::SPACING_LARGE - 1.0f;
    
    // Description with curY update
    UIWidgets::drawTextWrapped(element.description.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, WHITE);
}

void Inspector::drawElementCard(const Element& element, float x, float y, float size, InputHandler& input) {
    UIWidgets::drawElementCard(element, x, y, size, input);
}

void Inspector::drawMoleculeOverlay(Rectangle rect, InputHandler& input) {
    if (!currentMolecule) return;

    float curX = rect.x + UIConfig::INNER_PADDING;
    float curY = rect.y + UIConfig::HEADER_HEIGHT + 4.0f;
    float innerWidth = rect.width - (UIConfig::INNER_PADDING * 2.0f);

    // Formula and ID
    DrawText(LocalizationManager::getInstance().get("ui.inspector.structural_analysis").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_LABEL, GOLD);
    curY += 15.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    // Visual Card (Use molecule ID instead of symbol)
    UIWidgets::drawPanel({ curX, curY, UIConfig::INSPECTOR_CARD_SIZE, UIConfig::INSPECTOR_CARD_SIZE }, input, currentMolecule->color);
    DrawText(currentMolecule->id.c_str(), (int)curX + 10, (int)curY + 15, 20, WHITE);
    
    DrawText(currentMolecule->name.c_str(), (int)curX + (int)UIConfig::INSPECTOR_CARD_SIZE + 7, (int)curY + 2, UIConfig::FONT_SIZE_HEADER, WHITE);
    DrawText(currentMolecule->category.c_str(), (int)curX + (int)UIConfig::INSPECTOR_CARD_SIZE + 7, (int)curY + 15, UIConfig::FONT_SIZE_LABEL, GRAY);
    
    curY += UIConfig::INSPECTOR_CARD_SIZE + 12.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    DrawText(LocalizationManager::getInstance().get("ui.inspector.biological").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, Fade(currentMolecule->color, 0.8f));
    curY += 12.0f;
    UIWidgets::drawTextWrapped(currentMolecule->biologicalSignificance.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, WHITE);

    curY += 10.0f;
    DrawText(LocalizationManager::getInstance().get("ui.inspector.synthesis").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, SKYBLUE);
    curY += 14.0f;
    UIWidgets::drawTextWrapped(currentMolecule->description.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, Fade(WHITE, 0.9f));
}

void Inspector::drawTransitoryMoleculeOverlay(Rectangle rect, InputHandler& input) {
    float curX = rect.x + UIConfig::INNER_PADDING;
    float curY = rect.y + UIConfig::HEADER_HEIGHT + 4.0f;
    float innerWidth = rect.width - (UIConfig::INNER_PADDING * 2.0f);

    DrawText(LocalizationManager::getInstance().get("ui.inspector.transitory_status").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_LABEL, SKYBLUE);
    curY += 15.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    DrawText(LocalizationManager::getInstance().get("ui.inspector.composition").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, GRAY);
    curY += 15.0f;

    for (auto const& [atomicNum, count] : currentComposition) {
        const Element& el = ChemistryDatabase::getInstance().getElement(atomicNum);
        UIWidgets::drawValueLabel(el.name.c_str(), TextFormat("x%d", count), curX, curY, innerWidth);
        curY += 2.0f; // Small adjustment between labels
    }

    curY += 10.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 10.0f;

    DrawText(LocalizationManager::getInstance().get("ui.inspector.primordial_analysis").c_str(), (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, GOLD);
    curY += 12.0f;
    UIWidgets::drawTextWrapped(LocalizationManager::getInstance().get("ui.inspector.unknown_desc").c_str(), 
                               curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, Fade(WHITE, 0.8f));

    curY = rect.y + rect.height - 30;
}

