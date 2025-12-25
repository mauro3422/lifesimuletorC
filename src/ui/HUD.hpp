#ifndef HUD_HPP
#define HUD_HPP

#include "raylib.h"
#include "../input/InputHandler.hpp"

namespace HUD {
    void draw(const Camera2D& camera, bool freeMode, InputHandler& input);
}

#endif // HUD_HPP
