#ifndef UI_WIDGETS_HPP
#define UI_WIDGETS_HPP

#include "raylib.h"
#include "../core/Config.hpp"
#include "../input/InputHandler.hpp"
#include "../chemistry/Element.hpp"
#include <vector>
#include <string>

/**
 * UIWidgets: Dynamic and premium UI system for LifeSimulator C++.
 * Manages layouts, input capture, and advanced visual effects.
 */
class UIWidgets {
public:
    // Draws a panel with a subtle outer glow effect
    static void drawPanel(Rectangle rect, InputHandler& input, Color accentColor = Config::THEME_BORDER);

    // Dynamic header - vertically centered elements with horizontal padding
    static void drawHeader(Rectangle panelRect, const char* title, Color color = Config::THEME_BORDER);

    // Centralized interactive button
    static bool drawButton(Rectangle rect, const char* label, InputHandler& input, Color accent = Config::THEME_BORDER);

    // Unified Element Card (For Inspector and Quimidex)
    static void drawElementCard(const Element& element, float x, float y, float size, InputHandler& input);

    // Draws wrapped text and updates the Y pointer (Resolves overflows)
    static void drawTextWrapped(const char* text, float x, float& y, float maxWidth, int fontSize, Color color);

    static void drawProgressBar(Rectangle rect, float progress, Color color, const char* label = "");

    static void drawValueLabel(const char* key, const char* value, float x, float& y, float width, Color accent = Config::THEME_TEXT_SECONDARY);

    static void drawSeparator(float x, float y, float width);

    // Tab System
    static int drawTabSystem(Rectangle rect, const std::vector<std::string>& labels, int activeIndex, InputHandler& input, Color accent = Config::THEME_HIGHLIGHT);

    // Selection list (for Inventories/Quimidex)
    static int drawListSelection(Rectangle rect, const std::vector<std::string>& items, int activeIndex, InputHandler& input, Color accent = Config::THEME_HIGHLIGHT);
};

#endif
