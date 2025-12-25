#include "Inspector.hpp"
#include "UIWidgets.hpp"
#include "../core/Config.hpp"
#include <string>

Inspector::Inspector() {}

void Inspector::draw(const Element& element, int entityID, InputHandler& input) {
    int screenH = GetScreenHeight();
    
    // CONFIGURACIÓN DE LAYOUT
    float margin = (float)Config::INSPECTOR_MARGIN;
    float width = (float)Config::INSPECTOR_WIDTH;
    float x = margin;
    float innerWidth = width - 16.0f;
    
    // --- PASO 1: CALCULAR ALTURA EXACTA (PRE-PASS) ---
    float calculatedHeight = 24.0f; // Header
    calculatedHeight += 15.0f;      // ID line
    calculatedHeight += 8.0f;       // Separador
    calculatedHeight += 62.0f;      // Element Card area
    calculatedHeight += 8.0f;       // Separador
    
    // Datos técnicos
    calculatedHeight += (10.0f + 5.0f) * 4; // 4 rows
    
    // Origen (Dynamic)
    float tempY = 0;
    UIWidgets::drawTextWrapped(element.origin.c_str(), 0, tempY, innerWidth, 10, BLANK); 
    calculatedHeight += 12.0f + tempY; // Label + Text
    
    // Lore Header + Lore (Dynamic)
    calculatedHeight += 6.0f + 14.0f;
    tempY = 0;
    UIWidgets::drawTextWrapped(element.description.c_str(), 0, tempY, innerWidth, 10, BLANK);
    calculatedHeight += tempY;
    
    // Botón
    calculatedHeight += 30.0f; 

    // Rectángulo final ajustado al contenido
    Rectangle rect = { x, (float)(screenH - calculatedHeight - margin), width, calculatedHeight };

    // 1. Fondo Premium con Glow
    UIWidgets::drawPanel(rect, input, element.color);
    UIWidgets::drawHeader(rect, TextFormat("[+] %s", element.name.c_str()), element.color);

    float curX = rect.x + 8.0f;
    float curY = rect.y + 24.0f;

    // 2. ID y Separador
    DrawText(TextFormat(">> %s (ID: %d)", element.name.c_str(), entityID), (int)curX, (int)curY, 10, SKYBLUE);
    curY += 15.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    // 3. Tarjeta de Elemento y Valencias
    drawElementCard(element, curX, curY, 50.0f, input);
    
    int infoX = (int)curX + 57;
    int infoY = (int)curY + 2;
    DrawText(element.name.c_str(), infoX, infoY, 12, WHITE);
    DrawText(TextFormat("[%s] #%d", element.symbol.c_str(), element.atomicNumber), infoX, infoY + 15, 10, LIGHTGRAY);
    
    // Barra de Valencia Dinámica
    UIWidgets::drawProgressBar({ (float)infoX, (float)infoY + 32, 60, 6 }, 0.0f, element.color, TextFormat("0/%d", element.maxBonds));

    curY += 62.0f;
    UIWidgets::drawSeparator(curX, curY, innerWidth);
    curY += 8.0f;

    // 4. Datos Técnicos (Actualizan curY automáticamente)
    UIWidgets::drawValueLabel("Electroneg.", TextFormat("%.2f", element.electronegativity), curX, curY, innerWidth);
    UIWidgets::drawValueLabel("VdW Radius", TextFormat("%d pm", (int)element.vdWRadius), curX, curY, innerWidth);
    UIWidgets::drawValueLabel("Atomic Mass", TextFormat("%.1f u", element.atomicMass), curX, curY, innerWidth);
    UIWidgets::drawValueLabel("Max Bonds", TextFormat("%d", element.maxBonds), curX, curY, innerWidth);
    
    // EL ORIGEN AHORA USA drawTextWrapped para evitar overlap
    DrawText("ORIGIN:", (int)curX, (int)curY, 9, Config::THEME_TEXT_SECONDARY);
    curY += 12.0f;
    UIWidgets::drawTextWrapped(element.origin.c_str(), curX, curY, innerWidth, 10, ColorAlpha(SKYBLUE, 0.8f));

    curY += 6.0f;
    DrawText("LORE & DATA", (int)curX, (int)curY, 9, SKYBLUE);
    curY += 14.0f;
    
    // Descripción con actualización de curY
    UIWidgets::drawTextWrapped(element.description.c_str(), curX, curY, innerWidth, 10, WHITE);

    // 5. Botón ANALYZE (Siempre al final del flujo)
    curY += 5.0f;
    Rectangle btnRect = { rect.x + 5, curY, rect.width - 10, 20 };
    if (UIWidgets::drawButton(btnRect, "ANALYZE MOLECULE", input, element.color)) {
        TraceLog(LOG_INFO, "User requested deep molecule analysis");
    }
    
    // Ajuste final del panel si el botón se sale (para el siguiente frame o evitar corte visual)
    // Nota: En Raylib inmediato, si calculamos la altura DESPUÉS de dibujar el panel, queda mal un frame.
    // Pero con el rectHeight estimado arriba debería bastar.
}

void Inspector::drawElementCard(const Element& element, float x, float y, float size, InputHandler& input) {
    UIWidgets::drawPanel((Rectangle){ x, y, size, size }, input, element.color);
    DrawText(element.symbol.c_str(), (int)x + (size/2 - MeasureText(element.symbol.c_str(), 20)/2), (int)y + 8, 20, element.color);
    DrawText(TextFormat("%.1f", element.atomicMass), (int)x + 5, (int)y + (int)size - 12, 9, WHITE);
}
