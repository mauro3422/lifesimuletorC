#include "HUD.hpp"
#include "UIWidgets.hpp"
#include "../core/Config.hpp"
#include "raylib.h"
#include "../core/LocalizationManager.hpp"
#include <cstdio>

namespace HUD {
    void draw(const Camera2D& camera, bool freeMode, InputHandler& input) {
        Rectangle hudRect = { 0, 0, (float)GetScreenWidth(), (float)Config::HUD_HEIGHT };
        auto& lm = LocalizationManager::getInstance();
        UIWidgets::drawPanel(hudRect, input, Fade(Config::THEME_BORDER, 0.3f));
        
        DrawFPS(10, 5);
        DrawText("LifeSimulator C++ | LORE-CORE", 10, 20, Config::HUD_FONT_TITLE, Config::THEME_HIGHLIGHT);
        
        const char* modeText = freeMode ? lm.get("ui.hud.mode_free").c_str() : lm.get("ui.hud.mode_follow").c_str();
        DrawText(modeText, 10, 40, Config::HUD_FONT_INFO, freeMode ? Config::THEME_WARNING : Config::THEME_TEXT_SECONDARY);
        
        char zoomText[50];
        std::sprintf(zoomText, lm.get("ui.hud.view_zoom").c_str(), camera.zoom);
        DrawText(zoomText, GetScreenWidth() - 110, 20, Config::HUD_FONT_ZOOM, Config::THEME_ACCENT);
        
        Rectangle helpRect = { (float)GetScreenWidth() - 85, (float)Config::HUD_HEIGHT - 25, 75, 18 };
        if (UIWidgets::drawButton(helpRect, lm.get("ui.hud.quimidex").c_str(), input)) {
            TraceLog(LOG_INFO, "Help Button Clicked!");
        }
    }
}
