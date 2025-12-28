#ifndef LOADING_SCREEN_HPP
#define LOADING_SCREEN_HPP

#include "raylib.h"
#include "UIWidgets.hpp"
#include "UIConfig.hpp"
#include "../core/Config.hpp"
#include <string>

/**
 * LoadingScreen: Manages visual transitions during heavy world initialization.
 */
class LoadingScreen {
public:
    LoadingScreen() : progress(0.0f), status("Initializing systems...") {}

    void draw(float targetProgress, const std::string& message) {
        // Smooth progress transition
        progress += (targetProgress - progress) * 0.1f;
        status = message;

        BeginDrawing();
        ClearBackground(Config::THEME_BACKDROP);

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // Logo / Central Title
        const char* title = "LIFE SIMULATOR";
        int titleSize = 40;
        int titleWidth = MeasureText(title, titleSize);
        DrawText(title, (screenW - titleWidth) / 2, screenH / 2 - 60, titleSize, Config::THEME_BORDER);

        const char* subTitle = "NANO-HD MOLECULAR ENGINE";
        int subTitleSize = 12;
        int subTitleWidth = MeasureText(subTitle, subTitleSize);
        DrawText(subTitle, (screenW - subTitleWidth) / 2, screenH / 2 - 20, subTitleSize, GRAY);

        // Barra de progreso
        float barWidth = 400.0f;
        float barHeight = 8.0f;
        Rectangle barRect = { (screenW - barWidth) / 2.0f, (screenH / 2.0f) + 20.0f, barWidth, barHeight };
        
        UIWidgets::drawProgressBar(barRect, progress, Config::THEME_BORDER);

        // Mensaje de estado
        int statusSize = 10;
        int statusWidth = MeasureText(status.c_str(), statusSize);
        DrawText(status.c_str(), (screenW - statusWidth) / 2, (int)barRect.y + 20, statusSize, Config::THEME_TEXT_SECONDARY);

        // Aesthetic micro-animation (Loading particles)
        float time = (float)GetTime();
        for (int i = 0; i < 5; i++) {
            float orbit = time * 2.0f + (i * 1.2f);
            float ox = screenW / 2.0f + cosf(orbit) * 50.0f;
            float oy = screenH / 2.0f + sinf(orbit) * 50.0f;
            DrawCircle((int)ox, (int)oy, 2.0f, Fade(Config::THEME_BORDER, 0.4f));
        }

        EndDrawing();
    }

    bool isFinished() const { return progress >= 0.99f; }

private:
    float progress;
    std::string status;
};

#endif
