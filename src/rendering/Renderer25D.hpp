#ifndef RENDERER_25D_HPP
#define RENDERER_25D_HPP

#include "raylib.h"
#include "ecs/components.hpp"
#include <vector>

/**
 * Renderer optimizado para 2.5D.
 * Implementación desacoplada en Renderer25D.cpp.
 */
class Renderer25D {
public:
    static void drawAtoms(const std::vector<TransformComponent>& transforms, 
                         const std::vector<AtomComponent>& atoms,
                         const std::vector<StateComponent>& states);
};

#endif
