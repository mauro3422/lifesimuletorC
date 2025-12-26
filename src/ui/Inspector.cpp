#include "Inspector.hpp"
#include "UIWidgets.hpp"
#include "UIConfig.hpp"
#include "../core/Config.hpp"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../gameplay/DiscoveryLog.hpp"
#include "../gameplay/MissionManager.hpp"
#include <string>

Inspector::Inspector() {}

void Inspector::draw(const Element& element, int entityID, InputHandler& input, std::vector<StateComponent>& states, std::vector<AtomComponent>& atoms) {
    int screenH = GetScreenHeight();
    
    // CONFIGURACIÓN DE LAYOUT
    float margin = (float)Config::INSPECTOR_MARGIN;
    float width = UIConfig::INSPECTOR_WIDTH;
    float x = margin;
    float innerWidth = width - (UIConfig::INNER_PADDING * 2.0f);

    Color activeColor = currentMolecule ? currentMolecule->color : element.color;
    // Si es un clúster pero NO una molécula reconocida, usamos un color neutro o el del elemento detectado
    if (!currentMolecule && states[entityID].parentEntityId != -1) activeColor = element.color;
    
    // --- PASO 1: CALCULAR ALTURA ESTIMADA ---
    float calculatedHeight = 40.0f; // Margen base (header + padding)
    
    if (currentMolecule) {
        calculatedHeight = 260.0f; // Moleculas suelen ser fijas pero largas
    } else {
        int totalAtoms = 0;
        for (auto const& [num, count] : currentComposition) totalAtoms += count;
        
        if (totalAtoms > 1) {
            // Clúster: Header + Status + Sep + CompH + (N * 15) + Sep + ObsH + Obs + Footer
            calculatedHeight = UIConfig::HEADER_HEIGHT + 15 + 8 + 15 + (currentComposition.size() * UIConfig::LIST_ITEM_HEIGHT) + 20 + 12 + 50 + 40;
        } else {
            calculatedHeight = (float)Config::INSPECTOR_HEIGHT;
        }
    }

    Rectangle rect = { x, (float)(screenH - calculatedHeight - margin), width, calculatedHeight };

    // 1. Fondo Premium con Glow
    UIWidgets::drawPanel(rect, input, activeColor);
    
    std::string headerTitle = currentMolecule ? TextFormat("[M] %s", currentMolecule->name.c_str()) : TextFormat("[+] %s", element.name.c_str());
    
    // Si no es molécula pero tiene más de 1 átomo, es un clúster transitorio
    int totalAtoms = 0;
    for (auto const& [num, count] : currentComposition) totalAtoms += count;

    if (!currentMolecule && totalAtoms > 1) {
        headerTitle = TextFormat("[C] Cluster de %s", element.symbol.c_str());
    }
    
    UIWidgets::drawHeader(rect, headerTitle.c_str(), activeColor);

    // Si estamos en modo clúster (detectado por tener composición > 1 átomo)
    // o si específicamente tenemos una molécula reconocida.
    if (currentMolecule) {
        drawMoleculeOverlay(rect, input);
        return; 
    } else if (totalAtoms > 1) {
        drawTransitoryMoleculeOverlay(rect, input);
        return;
    }

    float curX = rect.x + UIConfig::INNER_PADDING;
    float curY = rect.y + UIConfig::HEADER_HEIGHT + UIConfig::SPACING_MEDIUM + 5.0f; // Padding extra para no quedar pegado al header

    // 2. Tarjeta de Elemento y Valencias (Sin línea de ID redundante)
    UIWidgets::drawElementCard(element, curX, curY, UIConfig::INSPECTOR_CARD_SIZE, input);
    
    int infoX = (int)curX + (int)UIConfig::INSPECTOR_CARD_SIZE + 7;
    int infoY = (int)curY + 2;
    DrawText(element.name.c_str(), infoX, infoY, UIConfig::FONT_SIZE_HEADER, WHITE);
    DrawText(TextFormat("[%s] #%d", element.symbol.c_str(), element.atomicNumber), infoX, infoY + 15, UIConfig::FONT_SIZE_LABEL, LIGHTGRAY);
    
    // Barra de Valencia Dinámica
    UIWidgets::drawProgressBar({ (float)infoX, (float)infoY + UIConfig::CARD_INFO_OFFSET_Y, 60, UIConfig::INSPECTOR_BAR_HEIGHT }, 0.0f, element.color, TextFormat("0/%d", element.maxBonds));

    curY += UIConfig::INSPECTOR_CARD_SIZE + UIConfig::SPACING_MEDIUM;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += UIConfig::SPACING_SMALL;

    // 4. Datos Técnicos (Actualizan curY automáticamente)
    UIWidgets::drawValueLabel("Electroneg.", TextFormat("%.2f", element.electronegativity), curX, curY, innerWidth);
    UIWidgets::drawValueLabel("VdW Radius", TextFormat("%d pm", (int)element.vdWRadius), curX, curY, innerWidth);
    UIWidgets::drawValueLabel("Atomic Mass", TextFormat("%.1f u", element.atomicMass), curX, curY, innerWidth);
    UIWidgets::drawValueLabel("Max Bonds", TextFormat("%d", element.maxBonds), curX, curY, innerWidth);
    
    // EL ORIGEN AHORA USA drawTextWrapped para evitar overlap
    DrawText("ORIGIN:", (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, Config::THEME_TEXT_SECONDARY);
    curY += UIConfig::SPACING_MEDIUM;
    UIWidgets::drawTextWrapped(element.origin.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, ColorAlpha(SKYBLUE, 0.8f));

    curY += UIConfig::SPACING_SMALL - 2.0f;
    DrawText("LORE & DATA", (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, SKYBLUE);
    curY += UIConfig::SPACING_LARGE - 1.0f;
    
    // Descripción con actualización de curY
    UIWidgets::drawTextWrapped(element.description.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, WHITE);

    // Ajuste final del panel
    
    // Ajuste final del panel si el botón se sale (para el siguiente frame o evitar corte visual)
    // Nota: En Raylib inmediato, si calculamos la altura DESPUÉS de dibujar el panel, queda mal un frame.
    // Pero con el rectHeight estimado arriba debería bastar.
}

void Inspector::drawElementCard(const Element& element, float x, float y, float size, InputHandler& input) {
    UIWidgets::drawElementCard(element, x, y, size, input);
}

void Inspector::drawMoleculeOverlay(Rectangle rect, InputHandler& input) {
    if (!currentMolecule) return;

    float curX = rect.x + UIConfig::INNER_PADDING;
    float curY = rect.y + UIConfig::HEADER_HEIGHT + 4.0f;
    float innerWidth = rect.width - (UIConfig::INNER_PADDING * 2.0f);

    // Fórmula e ID
    DrawText(TextFormat("ANÁLISIS ESTRUCTURAL: %s", currentMolecule->formula.c_str()), (int)curX, (int)curY, UIConfig::FONT_SIZE_LABEL, GOLD);
    curY += 15.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    // Visual Card (Usamos ID de molécula en lugar de símbolo)
    UIWidgets::drawPanel({ curX, curY, UIConfig::INSPECTOR_CARD_SIZE, UIConfig::INSPECTOR_CARD_SIZE }, input, currentMolecule->color);
    DrawText(currentMolecule->id.c_str(), (int)curX + 10, (int)curY + 15, 20, WHITE);
    
    DrawText(currentMolecule->name.c_str(), (int)curX + (int)UIConfig::INSPECTOR_CARD_SIZE + 7, (int)curY + 2, UIConfig::FONT_SIZE_HEADER, WHITE);
    DrawText(currentMolecule->category.c_str(), (int)curX + (int)UIConfig::INSPECTOR_CARD_SIZE + 7, (int)curY + 15, UIConfig::FONT_SIZE_LABEL, GRAY);
    
    curY += UIConfig::INSPECTOR_CARD_SIZE + 12.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    DrawText("ROL PREBIÓTICO:", (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, Fade(currentMolecule->color, 0.8f));
    curY += 12.0f;
    UIWidgets::drawTextWrapped(currentMolecule->biologicalSignificance.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, WHITE);

    curY += 10.0f;
    DrawText("SÍNTESIS Y ORIGEN:", (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, SKYBLUE);
    curY += 14.0f;
    UIWidgets::drawTextWrapped(currentMolecule->description.c_str(), curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, Fade(WHITE, 0.9f));
}

void Inspector::drawTransitoryMoleculeOverlay(Rectangle rect, InputHandler& input) {
    float curX = rect.x + UIConfig::INNER_PADDING;
    float curY = rect.y + UIConfig::HEADER_HEIGHT + 4.0f;
    float innerWidth = rect.width - (UIConfig::INNER_PADDING * 2.0f);

    DrawText("ESTADO: MOLÉCULA TRANSITORIA", (int)curX, (int)curY, UIConfig::FONT_SIZE_LABEL, SKYBLUE);
    curY += 15.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    DrawText("COMPOSICIÓN DETECTADA:", (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, GRAY);
    curY += 15.0f;

    for (auto const& [atomicNum, count] : currentComposition) {
        const Element& el = ChemistryDatabase::getInstance().getElement(atomicNum);
        UIWidgets::drawValueLabel(el.name.c_str(), TextFormat("x%d", count), curX, curY, innerWidth);
        curY += 2.0f; // Pequeño ajuste entre labels
    }

    curY += 10.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 10.0f;

    DrawText("ANÁLISIS PRIMORDIAL:", (int)curX, (int)curY, UIConfig::FONT_SIZE_SMALL, GOLD);
    curY += 12.0f;
    UIWidgets::drawTextWrapped("Esta estructura no coincide con ninguna base de datos conocida. Representa una fase de alta energía o un precursor biótico aún no catalogado en la sopa primordial.", 
                               curX, curY, innerWidth, UIConfig::FONT_SIZE_LABEL, Fade(WHITE, 0.8f));

    curY = rect.y + rect.height - 30;
}
