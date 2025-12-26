#include "InputHandler.hpp"
#include "../core/Config.hpp"
#include <cmath>

InputHandler::InputHandler() 
    : tractorActive(false), panningActive(false), selectionTriggered(false), releaseTriggered(false),
      moveDir({0,0}), mousePos({0,0}), mouseDelta({0,0}), wheelMove(0), 
      mouseCapturedByUI(false) {}

void InputHandler::resetFrameState() {
    mouseCapturedByUI = false;
}

void InputHandler::update() {
    Vector2 oldPos = mousePos;
    mousePos = GetMousePosition();
    mouseDelta = { mousePos.x - oldPos.x, mousePos.y - oldPos.y };
    wheelMove = GetMouseWheelMove();

    // Movement Detection (WASD)
    moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S)) moveDir.y += 1;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D)) moveDir.x += 1;

    // Direction normalization
    if (moveDir.x != 0 || moveDir.y != 0) {
        float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        moveDir.x /= len;
        moveDir.y /= len;
    }

    // Base Mouse Detection
    bool leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool rightPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    bool middleDown = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    bool leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Only activate if NOT captured
    tractorActive = leftDown && !mouseCapturedByUI;
    panningActive = middleDown && !mouseCapturedByUI;
    selectionTriggered = leftPressed && !mouseCapturedByUI;
    releaseTriggered = rightPressed && !mouseCapturedByUI;
    
}

void InputHandler::setMouseCaptured(bool captured) {
    if (captured) {
        mouseCapturedByUI = true;
        tractorActive = false;
        panningActive = false;
        selectionTriggered = false;
        releaseTriggered = false;
    }
}

bool InputHandler::isMouseCaptured() const { return mouseCapturedByUI; }
bool InputHandler::isActionAllowed() const { return !mouseCapturedByUI; }

bool InputHandler::isTractorBeamActive() const { return tractorActive; }
bool InputHandler::isPanning() const { return panningActive; }
bool InputHandler::isSelectionTriggered() const { return selectionTriggered; }
bool InputHandler::isReleaseTriggered() const { return releaseTriggered; }

Vector2 InputHandler::getMovementDirection() const { return moveDir; }

Vector2 InputHandler::getMousePosition() const { return mousePos; }
Vector2 InputHandler::getMouseDelta() const { return mouseDelta; }
float InputHandler::getMouseWheelMove() const { return wheelMove; }
bool InputHandler::isSpaceTriggered() const { return IsKeyPressed(KEY_SPACE); }
bool InputHandler::isMouseOverUI() const { return mouseCapturedByUI; }
