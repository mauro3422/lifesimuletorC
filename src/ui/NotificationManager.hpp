#ifndef NOTIFICATION_MANAGER_HPP
#define NOTIFICATION_MANAGER_HPP

#include "raylib.h"
#include <string>

/**
 * Sistema simple de notificaciones en pantalla.
 * Muestra mensajes temporales (ej: "Enlace incompatible!")
 */
class NotificationManager {
public:
    static NotificationManager& getInstance() {
        static NotificationManager instance;
        return instance;
    }

    void show(const std::string& message, Color color = WHITE, float duration = 2.0f) {
        currentMessage = message;
        messageColor = color;
        timer = duration;
    }

    void update(float dt) {
        if (timer > 0) timer -= dt;
    }

    void draw() {
        if (timer <= 0) return;
        
        int screenW = GetScreenWidth();
        int fontSize = 18;
        int textW = MeasureText(currentMessage.c_str(), fontSize);
        
        // Fondo semi-transparente
        Rectangle bgRect = { 
            (float)(screenW / 2 - textW / 2 - 15), 
            60.0f, 
            (float)(textW + 30), 
            32.0f 
        };
        DrawRectangleRec(bgRect, Fade(BLACK, 0.7f));
        DrawRectangleLinesEx(bgRect, 1, Fade(messageColor, 0.5f));
        
        // Texto centrado
        DrawText(currentMessage.c_str(), screenW / 2 - textW / 2, 68, fontSize, messageColor);
    }

private:
    NotificationManager() : timer(0), messageColor(WHITE) {}
    std::string currentMessage;
    Color messageColor;
    float timer;
};

#endif
