#ifndef INSPECTOR_HPP
#define INSPECTOR_HPP

#include "chemistry/Element.hpp"
#include "input/InputHandler.hpp"
#include "raylib.h"

class Inspector {
public:
    Inspector();
    void draw(const Element& element, int entityID, InputHandler& input);

private:
    void drawElementCard(const Element& element, float x, float y, float size, InputHandler& input);
};

#endif
