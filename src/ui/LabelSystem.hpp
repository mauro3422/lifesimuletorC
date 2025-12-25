#ifndef LABEL_SYSTEM_HPP
#define LABEL_SYSTEM_HPP

#include "raylib.h"
#include "../ecs/components.hpp"
#include <vector>

/**
 * SISTEMA DE ETIQUETAS (LABELS)
 */
class LabelSystem {
public:
    static void draw(const Camera2D& camera, 
                     const std::vector<TransformComponent>& transforms, 
                     const std::vector<AtomComponent>& atoms);
};

#endif
