#ifndef UI_WIDGETS_HPP
#define UI_WIDGETS_HPP

#include "raylib.h"
#include "../core/Config.hpp"
#include "../input/InputHandler.hpp"
#include <string>
#include <vector>
#include <algorithm> // Requerido para std::clamp y std::max
#include "UIConfig.hpp"
#include "../chemistry/Element.hpp"

/**
 * UIWidgets: Sistema de UI dinámico y premium para LifeSimulator C++.
 * Gestiona layouts, captura de entrada y efectos visuales avanzados.
 */
class UIWidgets {
public:
    // Dibuja un panel con un sutil resplandor exterior (Glow Effect)
    static void drawPanel(Rectangle rect, InputHandler& input, Color accentColor = Config::THEME_BORDER) {
        if (CheckCollisionPointRec(input.getMousePosition(), rect)) {
            input.setMouseCaptured(true);
        }
        
        for (int i = 1; i <= 3; i++) {
            DrawRectangleRoundedLines((Rectangle){ rect.x - i, rect.y - i, rect.width + i*2, rect.height + i*2 }, 
                UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, 1.0f, Fade(accentColor, 0.1f / i));
        }

        DrawRectangleRounded(rect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, Config::THEME_BACKDROP);
        DrawRectangleRoundedLines(rect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, (float)Config::THEME_BORDER_WIDTH, accentColor);
    }

    // Cabecera dinámica
    static void drawHeader(Rectangle panelRect, const char* title, Color color = Config::THEME_BORDER) {
        float hHeight = UIConfig::HEADER_HEIGHT;
        DrawRectangleRec((Rectangle){ panelRect.x, panelRect.y, panelRect.width, hHeight }, Fade(color, Config::THEME_HEADER_OPACITY));
        DrawTriangle((Vector2){ panelRect.x + 5, panelRect.y + 4 }, (Vector2){ panelRect.x + 5, panelRect.y + 14 }, (Vector2){ panelRect.x + 12, panelRect.y + 9 }, WHITE);
        DrawText(title, (int)panelRect.x + 18, (int)panelRect.y + 3, UIConfig::FONT_SIZE_HEADER, WHITE);
    }

    // Botón interactivo centralizado
    static bool drawButton(Rectangle rect, const char* label, InputHandler& input, Color accent = Config::THEME_BORDER) {
        bool hovered = CheckCollisionPointRec(input.getMousePosition(), rect);
        if (hovered) input.setMouseCaptured(true);
        
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        Color base = hovered ? Fade(accent, 0.4f) : Fade(BLACK, 0.4f);
        DrawRectangleRounded(rect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, base);
        DrawRectangleRoundedLines(rect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, 1.0f, hovered ? Config::THEME_ACCENT : accent);
        
        int fontSize = UIConfig::FONT_SIZE_LABEL;
        int tWidth = MeasureText(label, fontSize);
        DrawText(label, (int)(rect.x + (rect.width - tWidth)/2), (int)(rect.y + (rect.height - fontSize)/2), fontSize, hovered ? Config::THEME_ACCENT : WHITE);
        
        return clicked;
    }

    // Ficha de Elemento unificada (Para Inspector y Quimidex)
    static void drawElementCard(const Element& element, float x, float y, float size, InputHandler& input) {
        UIWidgets::drawPanel((Rectangle){ x, y, size, size }, input, element.color);
        int symSize = (int)(size * 0.4f);
        DrawText(element.symbol.c_str(), (int)x + (int)(size/2 - MeasureText(element.symbol.c_str(), symSize)/2), (int)y + (int)(size * 0.15f), symSize, element.color);
        DrawText(TextFormat("%.1f", element.atomicMass), (int)x + 5, (int)y + (int)size - 12, UIConfig::FONT_SIZE_SMALL, element.color);
    }

    // Dibuja texto envuelto y actualiza el puntero Y (Resuelve desbordamientos)
    static void drawTextWrapped(const char* text, float x, float& y, float maxWidth, int fontSize, Color color) {
        std::string content = text;
        std::string currentLine = "";
        size_t start = 0;
        size_t end = content.find(' ');

        while (end != std::string::npos) {
            std::string word = content.substr(start, end - start);
            std::string testLine = currentLine + word + " ";
            if (MeasureText(testLine.c_str(), fontSize) > (int)maxWidth) {
                if (color.a > 0) DrawText(currentLine.c_str(), (int)x, (int)y, fontSize, color);
                y += fontSize + 3.0f;
                currentLine = word + " ";
            } else currentLine = testLine;
            start = end + 1;
            end = content.find(' ', start);
        }

        std::string lastWord = content.substr(start);
        std::string testLine = currentLine + lastWord;
        if (MeasureText(testLine.c_str(), fontSize) > (int)maxWidth) {
            DrawText(currentLine.c_str(), (int)x, (int)y, fontSize, color);
            y += fontSize + 3.0f;
            DrawText(lastWord.c_str(), (int)x, (int)y, fontSize, color);
        } else {
            DrawText(testLine.c_str(), (int)x, (int)y, fontSize, color);
        }
        y += fontSize + 5.0f; // Espaciado final
    }

    static void drawProgressBar(Rectangle rect, float progress, Color color, const char* label = "") {
        DrawRectangleRec(rect, Fade(BLACK, 0.5f));
        DrawRectangleLinesEx(rect, 1.0f, Fade(color, 0.3f));
        
        float clampedProgress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);
        float fillWidth = (rect.width - 2) * clampedProgress;
        
        if (fillWidth > 0) {
            DrawRectangleGradientH((int)rect.x + 1, (int)rect.y + 1, (int)fillWidth, (int)rect.height - 2, color, Fade(color, 0.5f));
        }
        if (label != nullptr && label[0] != '\0') {
            DrawText(label, (int)rect.x + (int)rect.width + 5, (int)rect.y, UIConfig::FONT_SIZE_SMALL, WHITE);
        }
    }

    static void drawValueLabel(const char* key, const char* value, float x, float& y, float width, Color accent = Config::THEME_TEXT_SECONDARY) {
        int fontSize = UIConfig::FONT_SIZE_LABEL;
        DrawText(key, (int)x, (int)y, fontSize, accent);
        int valWidth = MeasureText(value, fontSize);
        DrawText(value, (int)(x + width - valWidth), (int)y, fontSize, WHITE);
        y += fontSize + UIConfig::TEXT_LINE_SPACING;
    }

    static void drawSeparator(float x, float y, float width) {
        DrawLine((int)x, (int)y, (int)(x + width), (int)y, Fade(Config::THEME_TEXT_SECONDARY, UIConfig::SEPARATOR_OPACITY));
    }

    // Sistema de Pestañas (Tabs)
    static int drawTabSystem(Rectangle rect, const std::vector<std::string>& labels, int activeIndex, InputHandler& input, Color accent = Config::THEME_HIGHLIGHT) {
        float tabWidth = rect.width / labels.size();
        int newIndex = activeIndex;

        for (int i = 0; i < (int)labels.size(); i++) {
            Rectangle tabRect = { rect.x + i * tabWidth, rect.y, tabWidth, rect.height };
            bool hovered = CheckCollisionPointRec(input.getMousePosition(), tabRect);
            if (hovered) input.setMouseCaptured(true);

            bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            if (clicked) newIndex = i;

            bool active = (i == activeIndex);
            
            // Fondo
            DrawRectangleRec(tabRect, active ? Fade(accent, 0.2f) : (hovered ? Fade(WHITE, 0.05f) : BLANK));
            
            // Texto
            int fontSize = UIConfig::FONT_SIZE_LABEL;
            const char* labelStr = labels[i].c_str();
            int tWidth = MeasureText(labelStr, fontSize);
            DrawText(labelStr, (int)(tabRect.x + (tabWidth - tWidth)/2), (int)(tabRect.y + (tabRect.height - fontSize)/2), fontSize, active ? accent : (hovered ? WHITE : Config::THEME_TEXT_SECONDARY));

            // Indicador activo
            if (active) {
                DrawRectangleRec((Rectangle){ tabRect.x, tabRect.y + tabRect.height - 2, tabRect.width, 2 }, accent);
            }
            
            // Separador vertical
            if (i < (int)labels.size() - 1) {
                DrawLine((int)(tabRect.x + tabWidth), (int)tabRect.y + 4, (int)(tabRect.x + tabWidth), (int)(tabRect.y + tabRect.height - 4), Fade(Config::THEME_BORDER, 0.5f));
            }
        }
        return newIndex;
    }

    // Lista de selección (para Inventarios/Quimidex)
    static int drawListSelection(Rectangle rect, const std::vector<std::string>& items, int activeIndex, InputHandler& input, Color accent = Config::THEME_HIGHLIGHT) {
        float itemHeight = 20.0f;
        int newIndex = activeIndex;
        
        BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
        
        for (int i = 0; i < (int)items.size(); i++) {
            Rectangle itemRect = { rect.x, rect.y + i * itemHeight, rect.width, itemHeight };
            bool hovered = CheckCollisionPointRec(input.getMousePosition(), itemRect);
            if (hovered) input.setMouseCaptured(true);

            bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            if (clicked) newIndex = i;

            bool active = (i == activeIndex);

            if (active) DrawRectangleRec(itemRect, Fade(accent, 0.15f));
            else if (hovered) DrawRectangleRec(itemRect, Fade(WHITE, 0.05f));

            // Viñeta y texto
            DrawCircle((int)itemRect.x + 8, (int)itemRect.y + 10, 2, active ? accent : Config::THEME_TEXT_SECONDARY);
            DrawText(items[i].c_str(), (int)itemRect.x + 18, (int)itemRect.y + 5, 10, active ? WHITE : (hovered ? LIGHTGRAY : Config::THEME_TEXT_SECONDARY));
        }
        
        EndScissorMode();
        return newIndex;
    }
};

#endif
