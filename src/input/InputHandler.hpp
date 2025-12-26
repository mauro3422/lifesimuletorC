#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "raylib.h"

/**
 * Módulo para gestionar las entradas del usuario (Teclado/Mouse).
 * Centraliza la detección y previene conflictos entre UI y Mundo.
 */
class InputHandler {
public:
    InputHandler();

    void update();
    void resetFrameState(); 

    // Gestión de Captura (UI vs Mundo)
    void setMouseCaptured(bool captured);
    bool isMouseCaptured() const;
    bool isMouseOverUI() const; 
    bool isActionAllowed() const;

    // Acciones del Mundo (Sujetas a captura)
    bool isTractorBeamActive() const;
    bool isPanning() const;
    bool isSelectionTriggered() const;
    bool isReleaseTriggered() const;
    
    // Movimiento (WASD)
    Vector2 getMovementDirection() const;

    // Datos de Mouse (Siempre disponibles)
    Vector2 getMousePosition() const;
    Vector2 getMouseDelta() const;
    float getMouseWheelMove() const;

    // Teclado
    bool isSpaceTriggered() const;

private:
    bool tractorActive;
    bool panningActive;
    bool selectionTriggered;
    bool releaseTriggered;
    Vector2 moveDir;
    Vector2 mousePos;
    Vector2 mouseDelta;
    float wheelMove;
    
    bool mouseCapturedByUI; 
};

#endif
