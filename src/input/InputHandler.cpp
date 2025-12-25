#include "InputHandler.hpp"
#include <cmath>

InputHandler::InputHandler() 
    : tractorActive(false), panningActive(false), selectionTriggered(false),
      moveDir({0,0}), mousePos({0,0}), mouseDelta({0,0}), wheelMove(0), 
      mouseCapturedByUI(false), lastSpaceTime(0), spaceDoubleTriggered(false) {}

void InputHandler::resetFrameState() {
    mouseCapturedByUI = false;
}

void InputHandler::update() {
    Vector2 oldPos = mousePos;
    mousePos = GetMousePosition();
    mouseDelta = { mousePos.x - oldPos.x, mousePos.y - oldPos.y };
    wheelMove = GetMouseWheelMove();

    // Detección de movimiento (WASD)
    moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S)) moveDir.y += 1;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D)) moveDir.x += 1;

    // Normalización de dirección
    if (moveDir.x != 0 || moveDir.y != 0) {
        float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        moveDir.x /= len;
        moveDir.y /= len;
    }

    // Detección base de mouse
    bool leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool rightDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    bool leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Solo activamos si NO está capturado
    tractorActive = leftDown && !mouseCapturedByUI;
    panningActive = rightDown && !mouseCapturedByUI;
    selectionTriggered = leftPressed && !mouseCapturedByUI;
    
    // DEBUG
    if (leftDown) {
        TraceLog(LOG_DEBUG, "[INPUT] Click: captured=%d, tractorActive=%d", mouseCapturedByUI, tractorActive);
    }
    
    // DEBUG: Verificar si el click está siendo detectado
    if (leftDown) {
        TraceLog(LOG_DEBUG, "[INPUT] Click detectado. Captured: %d, TractorActive: %d", mouseCapturedByUI, tractorActive);
    }

    // Lógica de Doble Espacio
    spaceDoubleTriggered = false;
    if (IsKeyPressed(KEY_SPACE)) {
        float currentTime = (float)GetTime();
        if (currentTime - lastSpaceTime < 0.3f) {
            spaceDoubleTriggered = true;
        }
        lastSpaceTime = currentTime;
    }
}

void InputHandler::setMouseCaptured(bool captured) {
    if (captured) {
        mouseCapturedByUI = true;
        tractorActive = false;
        panningActive = false;
        selectionTriggered = false;
    }
}

bool InputHandler::isMouseCaptured() const { return mouseCapturedByUI; }
bool InputHandler::isActionAllowed() const { return !mouseCapturedByUI; }

bool InputHandler::isTractorBeamActive() const { return tractorActive; }
bool InputHandler::isPanning() const { return panningActive; }
bool InputHandler::isSelectionTriggered() const { return selectionTriggered; }

Vector2 InputHandler::getMovementDirection() const { return moveDir; }

Vector2 InputHandler::getMousePosition() const { return mousePos; }
Vector2 InputHandler::getMouseDelta() const { return mouseDelta; }
float InputHandler::getMouseWheelMove() const { return wheelMove; }
bool InputHandler::isSpaceTriggered() const { return IsKeyPressed(KEY_SPACE); }
bool InputHandler::isSpaceDoubleTriggered() const { return spaceDoubleTriggered; }
bool InputHandler::isMouseOverUI() const { return mouseCapturedByUI; }
