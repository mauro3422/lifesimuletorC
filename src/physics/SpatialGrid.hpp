#ifndef SPATIAL_GRID_HPP
#define SPATIAL_GRID_HPP

#include "raylib.h"
#include "../ecs/components.hpp"
#include <vector>
#include <unordered_map>

/**
 * SPATIAL GRID (Grid Hash)
 * Divide el espacio en celdas para que las búsquedas sean O(1) en promedio.
 * Optimiza colisiones, tractor beam y enlaces moleculares.
 */
class SpatialGrid {
public:
    SpatialGrid(float cellSize);

    // Limpia la grilla y re-inserta todas las entidades
    void update(const std::vector<TransformComponent>& transforms);

    // Get entities in neighboring cells to a position
    std::vector<int> getNearby(Vector2 pos, float radius) const;

    // Helper for visual debugging
    void debugDraw() const;

private:
    float cellSize;
    
    struct Cell {
        std::vector<int> entityIndices;
    };

    // Using a simple Hash system to not depend on fixed limits
    long long getHash(int cx, int cy) const {
        return ((long long)cx << 32) | (unsigned int)cy;
    }

    // Almacenamiento de celdas
    std::unordered_map<long long, Cell> cells;
};

#endif
