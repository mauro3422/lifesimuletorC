#ifndef UI_WIDGETS_HPP
#define UI_WIDGETS_HPP

#include "raylib.h"
#include "../core/Config.hpp"
#include "../input/InputHandler.hpp"
#include <string>
#include <vector>
#include <algorithm> // Required for std::clamp and std::max
#include "UIConfig.hpp"
#include "../chemistry/Element.hpp"
#include <cmath>
#include <algorithm>

/**
 * UIWidgets: Dynamic and premium UI system for LifeSimulator C++.
 * Manages layouts, input capture, and advanced visual effects.
 */
class UIWidgets {
public:
    // Draws a panel with a subtle outer glow effect
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

    // Dynamic header - vertically centered elements with horizontal padding
    static void drawHeader(Rectangle panelRect, const char* title, Color color = Config::THEME_BORDER) {
        float hHeight = UIConfig::HEADER_HEIGHT;
        float vCenter = panelRect.y + hHeight / 2;
        
        // Calculate the ABSOLUTE pixel radius of the original panel to ensure the bleed matches perfectly
        float panelMin = (panelRect.width < panelRect.height) ? panelRect.width : panelRect.height;
        float absRadius = UIConfig::PANEL_ROUNDNESS * (panelMin / 2.0f);
        
        // Robust Scissor: We start 15px above and around the panel to allow the 4.0f bleed 
        // to fully overlap the top/side borders, but we cut strictly at the header bottom.
        int scissorX = (int)floor(panelRect.x - 15);
        int scissorY = (int)floor(panelRect.y - 15);
        int scissorW = (int)ceil(panelRect.width + 30);
        int scissorH = (int)ceil(hHeight + 15); // +15 to account for starting 15px above y
        
        BeginScissorMode(scissorX, scissorY, scissorW, scissorH);
        
        // Use a 4.0f bleed for robust overlap. 
        // We MUST calculate a new 'roundness' factor for this larger rect to maintain the SAME absolute radius
        Rectangle bleedRect = { panelRect.x - 4.0f, panelRect.y - 4.0f, panelRect.width + 8.0f, panelRect.height + 8.0f };
        float bleedMin = (bleedRect.width < bleedRect.height) ? bleedRect.width : bleedRect.height;
        float bleedRoundness = (absRadius * 2.0f) / bleedMin;
        
        DrawRectangleRounded(bleedRect, bleedRoundness, UIConfig::PANEL_SEGMENTS, Fade(color, Config::THEME_HEADER_OPACITY));
        
        // Redraw border lines for the header area to ensure they are on top and crisp
        DrawRectangleRoundedLines(panelRect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, (float)Config::THEME_BORDER_WIDTH, color);
        
        EndScissorMode();
        
        // Triangle indicator (centered vertically, padded from left)
        float triSize = 6.0f;
        float triX = panelRect.x + 18;  // Generous padding from left border
        DrawTriangle(
            (Vector2){ triX, vCenter - triSize/2 },      // Top
            (Vector2){ triX, vCenter + triSize/2 },      // Bottom  
            (Vector2){ triX + 6, vCenter },              // Right point
            WHITE
        );
        
        // Title text (centered vertically, more padding from triangle)
        int fontSize = UIConfig::FONT_SIZE_HEADER;
        DrawText(title, (int)panelRect.x + 28, (int)(vCenter - fontSize/2), fontSize, WHITE);
    }

    // Centralized interactive button
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

    // Unified Element Card (For Inspector and Quimidex)
    // Now uses element.backgroundColor for card background (two-tone effect)
    static void drawElementCard(const Element& element, float x, float y, float size, InputHandler& input) {
        Rectangle cardRect = { x, y, size, size };
        
        // Background with element's custom backgroundColor
        DrawRectangleRounded(cardRect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, element.backgroundColor);
        DrawRectangleRoundedLines(cardRect, UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, 1.0f, element.color);
        
        // Glow effect
        for (int i = 1; i <= 2; i++) {
            DrawRectangleRoundedLines((Rectangle){ x - i, y - i, size + i*2, size + i*2 }, 
                UIConfig::PANEL_ROUNDNESS, UIConfig::PANEL_SEGMENTS, 1.0f, Fade(element.color, 0.15f / i));
        }
        
        // Symbol centered
        int symSize = (int)(size * 0.4f);
        int symWidth = MeasureText(element.symbol.c_str(), symSize);
        DrawText(element.symbol.c_str(), (int)x + (int)(size/2 - symWidth/2), (int)y + (int)(size * 0.15f), symSize, element.color);
        
        // Atomic mass bottom-left
        DrawText(TextFormat("%.1f", element.atomicMass), (int)x + 5, (int)y + (int)size - 12, UIConfig::FONT_SIZE_SMALL, element.color);
        
        // Capture input
        if (CheckCollisionPointRec(input.getMousePosition(), cardRect)) {
            input.setMouseCaptured(true);
        }
    }

    // Draws wrapped text and updates the Y pointer (Resolves overflows)
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
        y += fontSize + 5.0f; // Final spacing
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

    // Tab System
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
            
            // Backdrop
            DrawRectangleRec(tabRect, active ? Fade(accent, 0.2f) : (hovered ? Fade(WHITE, 0.05f) : BLANK));
            
            // Text
            int fontSize = UIConfig::FONT_SIZE_LABEL;
            const char* labelStr = labels[i].c_str();
            int tWidth = MeasureText(labelStr, fontSize);
            DrawText(labelStr, (int)(tabRect.x + (tabWidth - tWidth)/2), (int)(tabRect.y + (tabRect.height - fontSize)/2), fontSize, active ? accent : (hovered ? WHITE : Config::THEME_TEXT_SECONDARY));

            // Active indicator
            if (active) {
                DrawRectangleRec((Rectangle){ tabRect.x, tabRect.y + tabRect.height - 2, tabRect.width, 2 }, accent);
            }
            
            // Vertical separator
            if (i < (int)labels.size() - 1) {
                DrawLine((int)(tabRect.x + tabWidth), (int)tabRect.y + 4, (int)(tabRect.x + tabWidth), (int)(tabRect.y + tabRect.height - 4), Fade(Config::THEME_BORDER, 0.5f));
            }
        }
        return newIndex;
    }

    // Selection list (for Inventories/Quimidex)
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

            // Bullet and text
            DrawCircle((int)itemRect.x + 8, (int)itemRect.y + 10, 2, active ? accent : Config::THEME_TEXT_SECONDARY);
            DrawText(items[i].c_str(), (int)itemRect.x + 18, (int)itemRect.y + 5, 10, active ? WHITE : (hovered ? LIGHTGRAY : Config::THEME_TEXT_SECONDARY));
        }
        
        EndScissorMode();
        return newIndex;
    }
};

#endif
