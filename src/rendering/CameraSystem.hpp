#ifndef CAMERA_SYSTEM_HPP
#define CAMERA_SYSTEM_HPP

#include "raylib.h"
#include "input/InputHandler.hpp"
#include "core/Config.hpp"
#include <cmath>
#include <algorithm>

/**
 * Sistema para gestionar el movimiento, zoom y modos de la cámara.
 * Incluye transiciones cinemáticas (Smooth Zoom).
 */
class CameraSystem {
public:
    enum Mode { FOLLOW_PLAYER, FREE_LOOK };

    CameraSystem() : currentMode(FOLLOW_PLAYER), targetZoom(Config::CAMERA_INITIAL_ZOOM) {}

    void update(Camera2D& camera, const InputHandler& input, Vector2 targetPos, float dt) {
        // 1. Cambio de Modo con Panning (Click derecho)
        if (input.isPanning()) {
            currentMode = FREE_LOOK;
            camera.target.x -= input.getMouseDelta().x / camera.zoom;
            camera.target.y -= input.getMouseDelta().y / camera.zoom;
            targetZoom = camera.zoom; // Sincronizamos para que no salte al dejar de pannear
        }

        // 2. Reset con Espacio (Inicia transición cinemática)
        if (input.isSpaceTriggered()) {
            currentMode = FOLLOW_PLAYER;
            targetZoom = Config::CAMERA_INITIAL_ZOOM; 
        }

        // 3. Lógica de Seguimiento Suave
        if (currentMode == FOLLOW_PLAYER) {
            camera.target.x += (targetPos.x - camera.target.x) * Config::CAMERA_FOLLOW_SPEED * dt;
            camera.target.y += (targetPos.y - camera.target.y) * Config::CAMERA_FOLLOW_SPEED * dt;
        }

        // 4. Zoom Global (Manual + Interpolado)
        if (!input.isMouseOverUI()) {
            float wheel = input.getMouseWheelMove();
            if (wheel != 0) {
                float scaleFactor = 1.0f + (Config::CAMERA_ZOOM_WHEEL_SENSITIVITY * std::abs(wheel));
                if (wheel < 0) targetZoom /= scaleFactor;
                else targetZoom *= scaleFactor;
            }
        }
        
        // Limitar Zoom Objetivo
        targetZoom = std::clamp(targetZoom, Config::CAMERA_ZOOM_MIN, Config::CAMERA_ZOOM_MAX);

        // --- TRANSICIÓN CINEMÁTICA (Lerp para el Zoom) ---
        camera.zoom += (targetZoom - camera.zoom) * Config::CAMERA_ZOOM_SMOOTH * dt;
    }

    Mode getMode() const { return currentMode; }

private:
    Mode currentMode;
    float targetZoom; // Zoom hacia el que queremos ir
};

#endif
