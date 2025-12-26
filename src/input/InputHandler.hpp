#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "raylib.h"

/**
 * Module to manage user input (Keyboard/Mouse).
 * Centralizes detection and prevents conflicts between UI and World.
 */
class InputHandler {
public:
    InputHandler();

    void update();
    void resetFrameState(); 

    // Capture Management (UI vs World)
    void setMouseCaptured(bool captured);
    bool isMouseCaptured() const;
    bool isMouseOverUI() const; 
    bool isActionAllowed() const;

    // World Actions (Subject to capture)
    bool isTractorBeamActive() const;
    bool isPanning() const;
    bool isSelectionTriggered() const;
    bool isReleaseTriggered() const;
    
    // Movement (WASD)
    Vector2 getMovementDirection() const;

    // Mouse Data (Always available)
    Vector2 getMousePosition() const;
    Vector2 getMouseDelta() const;
    float getMouseWheelMove() const;

    // Keyboard
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
