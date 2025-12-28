#ifndef UI_CONFIG_HPP
#define UI_CONFIG_HPP

#include "raylib.h"

/**
 * UIConfig: Centralized constants for UI system and layout design.
 */
namespace UIConfig {
    // --- LAYOUTS & PANELS ---
    inline constexpr float PANEL_ROUNDNESS = 0.15f;
    inline constexpr int PANEL_SEGMENTS = 32;
    inline constexpr float HEADER_HEIGHT = 25.0f;  // Compact header with centered content
    inline constexpr float MARGIN_DEFAULT = 10.0f;
    inline constexpr float INNER_PADDING = 8.0f;
    inline constexpr float SEPARATOR_THICKNESS = 1.0f;
    inline constexpr float SEPARATOR_OPACITY = 0.3f;

    // --- TYPOGRAPHY ---
    inline constexpr int FONT_SIZE_TITLE = 14;
    inline constexpr int FONT_SIZE_HEADER = 12;
    inline constexpr int FONT_SIZE_LABEL = 10;
    inline constexpr int FONT_SIZE_SMALL = 9;
    inline constexpr float TEXT_LINE_SPACING = 5.0f;

    // --- INSPECTOR ---
    inline constexpr float INSPECTOR_WIDTH = 220.0f;
    inline constexpr float INSPECTOR_CARD_SIZE = 50.0f;
    inline constexpr float INSPECTOR_BAR_HEIGHT = 6.0f;

    // --- QUIMIDEX ---
    inline constexpr float QUIMIDEX_WIDTH = 650.0f;
    inline constexpr float QUIMIDEX_HEIGHT = 450.0f;
    inline constexpr float QUIMIDEX_TAB_HEIGHT = 28.0f;
    inline constexpr float LIST_ITEM_HEIGHT = 22.0f;

    // --- SPACING (Replace magic numbers) ---
    inline constexpr float SPACING_TINY = 4.0f;
    inline constexpr float SPACING_SMALL = 8.0f;
    inline constexpr float SPACING_MEDIUM = 12.0f;
    inline constexpr float SPACING_LARGE = 15.0f;
    inline constexpr float SPACING_XL = 20.0f;
    inline constexpr float CARD_INFO_OFFSET_Y = 32.0f;  // Valencia bar position
    inline constexpr float LABEL_OFFSET = 2.0f;

    // --- DYNAMIC COLORS (Adapted from Config.hpp for enhanced visual control) ---
    inline const Color COLOR_SUCCESS = { 46, 204, 113, 255 };  // Emerald
    inline const Color COLOR_WARNING = { 241, 196, 15, 255 };  // Sunflower
    inline const Color COLOR_ERROR = { 231, 76, 60, 255 };     // Alizarin
    inline const Color COLOR_PRIMARY = { 52, 152, 219, 255 };  // Peter River
    inline const Color COLOR_SECONDARY = { 149, 165, 166, 255 }; // Asbestos
}

#endif
