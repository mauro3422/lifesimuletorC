#include "Quimidex.hpp"
#include "UIWidgets.hpp"
#include "UIConfig.hpp"
#include "../core/Config.hpp"

#include "../core/LocalizationManager.hpp"

Quimidex::Quimidex() {
    reload();
}

void Quimidex::reload() {
    auto& lm = LocalizationManager::getInstance();
    tabLabels = {
        lm.get("ui.quimidex.tab.molecules"),
        lm.get("ui.quimidex.tab.atoms"),
        lm.get("ui.quimidex.tab.progression")
    };
}

void Quimidex::draw(InputHandler& input) {
    if (!isOpen) return;

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Panel Central Grande
    float width = UIConfig::QUIMIDEX_WIDTH;
    float height = UIConfig::QUIMIDEX_HEIGHT;
    Rectangle rect = { (screenW - width) / 2, (screenH - height) / 2, width, height };

    UIWidgets::drawPanel(rect, input, Config::THEME_HIGHLIGHT);
    UIWidgets::drawHeader(rect, LocalizationManager::getInstance().get("ui.quimidex.title").c_str(), Config::THEME_HIGHLIGHT);

    // Close button (X) - vertically centered, with padding from edge
    float closeSize = 14.0f;
    float closeY = rect.y + (UIConfig::HEADER_HEIGHT - closeSize) / 2;
    if (UIWidgets::drawButton({ rect.x + rect.width - closeSize - 30, closeY, closeSize, closeSize }, "X", input, RED)) {
        isOpen = false;
    }

    // Tab System
    Rectangle tabRect = { rect.x + UIConfig::INNER_PADDING, rect.y + UIConfig::HEADER_HEIGHT + 4.0f, rect.width - (UIConfig::INNER_PADDING * 2.0f), UIConfig::QUIMIDEX_TAB_HEIGHT };
    activeTab = UIWidgets::drawTabSystem(tabRect, tabLabels, activeTab, input);

    Rectangle contentRect = { rect.x + UIConfig::INNER_PADDING, rect.y + UIConfig::HEADER_HEIGHT + UIConfig::QUIMIDEX_TAB_HEIGHT + 10.0f, rect.width - (UIConfig::INNER_PADDING * 2.0f), rect.height - (UIConfig::HEADER_HEIGHT + UIConfig::QUIMIDEX_TAB_HEIGHT + 20.0f) };
    
    switch (activeTab) {
        case 0: drawMoleculesTab(contentRect, input); break;
        case 1: drawAtomsTab(contentRect, input); break;
        case 2: drawProgressionTab(contentRect, input); break;
    }
}

void Quimidex::drawAtomsTab(Rectangle rect, InputHandler& input) {
    float listWidth = 150;
    Rectangle listRect = { rect.x, rect.y, listWidth, rect.height };
    Rectangle detailRect = { rect.x + listWidth + 10, rect.y, rect.width - listWidth - 10, rect.height };

    // Lista de Elementos
    std::vector<int> atomicNumbers = ChemistryDatabase::getInstance().getRegisteredAtomicNumbers();
    std::vector<std::string> names;
    for (int num : atomicNumbers) {
        names.push_back(ChemistryDatabase::getInstance().getElement(num).symbol + " - " + ChemistryDatabase::getInstance().getElement(num).name);
    }

    selectedElementIdx = UIWidgets::drawListSelection(listRect, names, selectedElementIdx, input);

    // Detalle
    if (selectedElementIdx < (int)atomicNumbers.size()) {
        drawAtomDetail(detailRect, ChemistryDatabase::getInstance().getElement(atomicNumbers[selectedElementIdx]), input);
    }
}

void Quimidex::drawAtomDetail(Rectangle rect, const Element& element, InputHandler& input) {
    auto& lm = LocalizationManager::getInstance();
    DrawText(lm.get("ui.quimidex.atom_detail").c_str(), (int)rect.x, (int)rect.y, UIConfig::FONT_SIZE_HEADER, LIME);
    UIWidgets::drawSeparator(rect.x, rect.y + 15, rect.width);

    float curY = rect.y + 25;
    
    // Unified Card
    UIWidgets::drawElementCard(element, rect.x, curY, 60.0f, input); 

    DrawText(element.name.c_str(), (int)rect.x + 70, (int)curY, 18, WHITE);
    DrawText(TextFormat("[%s] %s %d", element.symbol.c_str(), lm.get("ui.quimidex.atomic_number").c_str(), element.atomicNumber), (int)rect.x + 70, (int)curY + 22, UIConfig::FONT_SIZE_HEADER, GRAY);

    curY += 75;
    UIWidgets::drawSeparator(rect.x, curY, rect.width);
    curY += 10;

    UIWidgets::drawValueLabel(lm.get("ui.quimidex.electronegativity").c_str(), TextFormat("%.2f", element.electronegativity), rect.x, curY, rect.width);
    UIWidgets::drawValueLabel(lm.get("ui.quimidex.vdw_radius").c_str(), TextFormat("%d pm", (int)element.vdWRadius), rect.x, curY, rect.width);
    UIWidgets::drawValueLabel(lm.get("ui.quimidex.atomic_mass").c_str(), TextFormat("%.2f u", element.atomicMass), rect.x, curY, rect.width);
    UIWidgets::drawValueLabel(lm.get("ui.quimidex.max_bonds").c_str(), TextFormat("%d", element.maxBonds), rect.x, curY, rect.width);

    DrawText(lm.get("ui.inspector.description").c_str(), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_SMALL, RED);
    curY += 15;
    UIWidgets::drawTextWrapped(element.description.c_str(), rect.x, curY, rect.width, UIConfig::FONT_SIZE_LABEL, WHITE);

    DrawText(TextFormat(" %s: %s", lm.get("ui.inspector.origin").c_str(), element.origin.c_str()), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_SMALL, GOLD);
    curY += 15;
}

void Quimidex::drawMoleculesTab(Rectangle rect, InputHandler& input) {
    float listWidth = 150;
    Rectangle listRect = { rect.x, rect.y, listWidth, rect.height };
    Rectangle detailRect = { rect.x + listWidth + 10, rect.y, rect.width - listWidth - 10, rect.height };

    const std::vector<Molecule>& dbMolecules = ChemistryDatabase::getInstance().getAllMolecules();
    
    std::vector<std::string> molNames;
    for (const auto& mol : dbMolecules) molNames.push_back(mol.name);

    selectedMoleculeIdx = UIWidgets::drawListSelection(listRect, molNames, selectedMoleculeIdx, input);

    if (selectedMoleculeIdx >= 0 && selectedMoleculeIdx < (int)dbMolecules.size()) {
        drawMoleculeDetail(detailRect, dbMolecules[selectedMoleculeIdx], input);
    }
}

void Quimidex::drawMoleculeDetail(Rectangle rect, const Molecule& molecule, InputHandler& input) {
    auto& lm = LocalizationManager::getInstance();
    DrawText(lm.get("ui.quimidex.structural_analysis").c_str(), (int)rect.x, (int)rect.y, UIConfig::FONT_SIZE_HEADER, SKYBLUE);
    UIWidgets::drawSeparator(rect.x, rect.y + 15, rect.width);

    float curY = rect.y + 25;
    
    // Icon
    UIWidgets::drawPanel({ rect.x, curY, 60, 60 }, input, molecule.color);
    DrawText(molecule.formula.c_str(), (int)rect.x + 5, (int)curY + 20, 15, molecule.color);

    DrawText(molecule.name.c_str(), (int)rect.x + 70, (int)curY, 18, WHITE);
    DrawText(TextFormat("%s %s | %s", lm.get("ui.quimidex.formula").c_str(), molecule.formula.c_str(), molecule.category.c_str()), (int)rect.x + 70, (int)curY + 22, UIConfig::FONT_SIZE_HEADER, GRAY);

    curY += 75;
    UIWidgets::drawSeparator(rect.x, curY, rect.width);
    curY += 10;

    DrawText(lm.get("ui.quimidex.history").c_str(), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_SMALL, GRAY); curY += 15;
    UIWidgets::drawTextWrapped(molecule.description.c_str(), rect.x, curY, rect.width, UIConfig::FONT_SIZE_LABEL, WHITE);

    DrawText(lm.get("ui.quimidex.confluence").c_str(), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_SMALL, LIME); curY += 15;
    UIWidgets::drawTextWrapped(molecule.biologicalSignificance.c_str(), rect.x, curY, rect.width, UIConfig::FONT_SIZE_LABEL, WHITE);
}

void Quimidex::drawProgressionTab(Rectangle rect, InputHandler& input) {
    float listWidth = 200;
    Rectangle listRect = { rect.x, rect.y, listWidth, rect.height };
    Rectangle detailRect = { rect.x + listWidth + UIConfig::INNER_PADDING, rect.y, rect.width - listWidth - UIConfig::INNER_PADDING, rect.height };

    const auto& missions = MissionManager::getInstance().getMissions();
    std::vector<std::string> mTitles;
    for (const auto& m : missions) mTitles.push_back(m.title);

    selectedMissionIdx = UIWidgets::drawListSelection(listRect, mTitles, selectedMissionIdx, input);

    if (selectedMissionIdx < (int)missions.size()) {
        drawMissionDetail(detailRect, missions[selectedMissionIdx]);
    }
}

void Quimidex::drawMissionDetail(Rectangle rect, const Mission& mission) {
    auto& lm = LocalizationManager::getInstance();
    DrawText(TextFormat("== %s ==", mission.title.c_str()), (int)rect.x, (int)rect.y, 14, WHITE);
    
    std::string statusLabel = lm.get("ui.quimidex.mission_status") + " ";
    std::string statusValue = "";
    Color statusColor = WHITE;
    switch(mission.status) {
        case MissionStatus::LOCKED: statusValue = lm.get("ui.quimidex.status.locked"); statusColor = GRAY; break;
        case MissionStatus::AVAILABLE: statusValue = lm.get("ui.quimidex.status.available"); statusColor = SKYBLUE; break;
        case MissionStatus::ACTIVE: statusValue = lm.get("ui.quimidex.status.active"); statusColor = GOLD; break;
        case MissionStatus::COMPLETED: statusValue = lm.get("ui.quimidex.status.completed"); statusColor = LIME; break;
    }
    DrawText((statusLabel + statusValue).c_str(), (int)rect.x, (int)rect.y + 20, UIConfig::FONT_SIZE_HEADER, statusColor);
    
    UIWidgets::drawSeparator(rect.x, rect.y + 40, rect.width);

    float curY = rect.y + 50;
    UIWidgets::drawTextWrapped(mission.description.c_str(), rect.x, curY, rect.width, UIConfig::FONT_SIZE_LABEL, WHITE);

    DrawText(lm.get("ui.quimidex.scientific_context").c_str(), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_SMALL, SKYBLUE); curY += 15;
    UIWidgets::drawTextWrapped(mission.scientificContext.c_str(), rect.x, curY, rect.width, UIConfig::FONT_SIZE_LABEL, WHITE);

    UIWidgets::drawSeparator(rect.x, curY, rect.width); curY += 10;
    DrawText(lm.get("ui.quimidex.reward").c_str(), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_SMALL, LIME); curY += 15;
    DrawText(mission.reward.c_str(), (int)rect.x, (int)curY, UIConfig::FONT_SIZE_HEADER, WHITE);
}
