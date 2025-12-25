#ifndef UI_WIDGETS_HPP
#define UI_WIDGETS_HPP

#include "raylib.h"
#include "../core/Config.hpp"
#include "../input/InputHandler.hpp"
#include <string>
#include <vector>
#include <algorithm> // Requerido para std::clamp y std::max

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
        
        // Sombra / Resplandor exterior (Glow)
        // Usamos DrawRectangleRoundedLines con grosor si Raylib lo soporta, 
        // o simplemente varias capas de opacidad baja.
        for (int i = 1; i <= 3; i++) {
            DrawRectangleRoundedLines((Rectangle){ rect.x - i, rect.y - i, rect.width + i*2, rect.height + i*2 }, 
                Config::THEME_ROUNDNESS, 10, 1.0f, Fade(accentColor, 0.1f / i));
        }

        DrawRectangleRounded(rect, Config::THEME_ROUNDNESS, 10, Config::THEME_BACKDROP);
        DrawRectangleRoundedLines(rect, Config::THEME_ROUNDNESS, 10, (float)Config::THEME_BORDER_WIDTH, accentColor);
    }

    // Cabecera dinámica
    static void drawHeader(Rectangle panelRect, const char* title, Color color = Config::THEME_BORDER) {
        float hHeight = 18.0f;
        DrawRectangleRec((Rectangle){ panelRect.x, panelRect.y, panelRect.width, hHeight }, Fade(color, Config::THEME_HEADER_OPACITY));
        DrawTriangle((Vector2){ panelRect.x + 5, panelRect.y + 4 }, (Vector2){ panelRect.x + 5, panelRect.y + 14 }, (Vector2){ panelRect.x + 12, panelRect.y + 9 }, WHITE);
        DrawText(title, (int)panelRect.x + 18, (int)panelRect.y + 3, 12, WHITE);
    }

    // Botón interactivo centralizado
    static bool drawButton(Rectangle rect, const char* label, InputHandler& input, Color accent = Config::THEME_BORDER) {
        bool hovered = CheckCollisionPointRec(input.getMousePosition(), rect);
        if (hovered) input.setMouseCaptured(true);
        
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        Color base = hovered ? Fade(accent, 0.4f) : Fade(BLACK, 0.4f);
        DrawRectangleRounded(rect, 0.2f, 8, base);
        DrawRectangleRoundedLines(rect, 0.2f, 8, 1.0f, hovered ? Config::THEME_ACCENT : accent);
        
        int fontSize = 10;
        int tWidth = MeasureText(label, fontSize);
        DrawText(label, (int)(rect.x + (rect.width - tWidth)/2), (int)(rect.y + (rect.height - fontSize)/2), fontSize, hovered ? Config::THEME_ACCENT : WHITE);
        
        return clicked;
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

    // Barra de progreso estilizada (para Valencia/Energía)
    static void drawProgressBar(Rectangle rect, float progress, Color color, const char* label = "") {
        DrawRectangleRec(rect, Fade(BLACK, 0.5f));
        DrawRectangleLinesEx(rect, 1.0f, Fade(color, 0.3f));
        
        // Fix: std::max(0.0f, std::min(1.0f, progress)) como alternativa a std::clamp si hay problemas
        float clampedProgress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);
        float fillWidth = (rect.width - 2) * clampedProgress;
        
        if (fillWidth > 0) {
            DrawRectangleGradientH((int)rect.x + 1, (int)rect.y + 1, (int)fillWidth, (int)rect.height - 2, color, Fade(color, 0.5f));
        }
        if (label != nullptr && label[0] != '\0') {
            DrawText(label, (int)rect.x + (int)rect.width + 5, (int)rect.y, 9, WHITE);
        }
    }

    static void drawValueLabel(const char* key, const char* value, float x, float& y, float width, Color accent = Config::THEME_TEXT_SECONDARY) {
        int fontSize = 10;
        DrawText(key, (int)x, (int)y, fontSize, accent);
        int valWidth = MeasureText(value, fontSize);
        DrawText(value, (int)(x + width - valWidth), (int)y, fontSize, WHITE);
        y += fontSize + 5.0f;
    }

    static void drawSeparator(float x, float y, float width) {
        DrawLine((int)x, (int)y, (int)(x + width), (int)y, Fade(Config::THEME_TEXT_SECONDARY, 0.3f));
    }
};

#endif
