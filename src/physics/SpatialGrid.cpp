#include "SpatialGrid.hpp"
#include <cmath>
#include "../core/ErrorHandling.hpp"

SpatialGrid::SpatialGrid(float size) : cellSize(size) {}

void SpatialGrid::update(const std::vector<TransformComponent>& transforms) {
    if (transforms.empty()) {
        ErrorHandler::handle(ErrorSeverity::WARNING, "SpatialGrid::update received empty transforms");
        return;
    }
    // Phase 29: Memory Reuse Optimization
    // Instead of destroying everything, clear vectors to keep capacity
    for (auto& pair : cells) {
        pair.second.entityIndices.clear();
    }

    // Periodic map reset to prevent stale bucket bloat
    static int frameCounter = 0;
    if (++frameCounter > 300) { // Every ~5 seconds
        cells.clear();
        frameCounter = 0;
    }

    for (int i = 0; i < (int)transforms.size(); i++) {
        int cx = (int)std::floor(transforms[i].x / cellSize);
        int cy = (int)std::floor(transforms[i].y / cellSize);
        cells[getHash(cx, cy)].entityIndices.push_back(i);
    }
}

std::vector<int> SpatialGrid::getNearby(Vector2 pos, float radius) const {
    std::vector<int> nearby;
    int minX = (int)std::floor((pos.x - radius) / cellSize);
    int maxX = (int)std::floor((pos.x + radius) / cellSize);
    int minY = (int)std::floor((pos.y - radius) / cellSize);
    int maxY = (int)std::floor((pos.y + radius) / cellSize);

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            auto it = cells.find(getHash(x, y));
            if (it != cells.end()) {
                const auto& indices = it->second.entityIndices;
                nearby.insert(nearby.end(), indices.begin(), indices.end());
            }
        }
    }
    return nearby;
}

void SpatialGrid::debugDraw() const {
    // Visualizes active grid cells for debugging
    for (auto const& [hash, cell] : cells) {
        int cx = (int)(hash >> 32);
        int cy = (int)(hash & 0xFFFFFFFF);
        DrawRectangleLines(cx * (int)cellSize, cy * (int)cellSize, (int)cellSize, (int)cellSize, Fade(LIME, 0.2f));
    }
}
