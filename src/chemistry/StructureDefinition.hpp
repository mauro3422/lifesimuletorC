#ifndef STRUCTURE_DEFINITION_HPP
#define STRUCTURE_DEFINITION_HPP

#include <string>
#include <vector>
#include <cmath>
#include "raylib.h"

struct StructureDefinition {
    std::string name;              // e.g., "carbon_square"
    int atomCount;                 // Minimum atoms to trigger
    int atomicNumber;              // Required element (e.g., 6 for Carbon)
    float targetAngle;             // Internal angle in radians
    float damping;                 // Internal stability damping (relative to center)
    float globalDamping;           // Damping for the whole structure's drift (e.g. 0.98f)
    float formationSpeed;          // Attraction force multiplier during formation
    float formationDamping;        // Damping during the assembly phase
    float maxFormationSpeed;       // Speed clamping during assembly
    float completionThreshold;     // Distance to trigger rigid locking
    float rotationOffset;          // Global rotation in radians
    bool isPlanar;                 // Force Z=0?
    bool instantFormation;         // true = snap immediately
    
    // Calculates the ideal vertex positions for a regular polygon around (0,0)
    // with a given side length (bond distance)
    std::vector<Vector2> getIdealOffsets(float bondDist) const {
        std::vector<Vector2> offsets;
        if (atomCount < 3) return offsets;

        // For a regular polygon with n sides:
        // Radius of circumcircle R = side / (2 * sin(PI/n))
        float angleStep = (2.0f * 3.1415926535f) / atomCount;
        float radius = bondDist / (2.0f * std::sin(3.1415926535f / atomCount));

        for (int i = 0; i < atomCount; i++) {
            float currentAngle = i * angleStep + rotationOffset;
            offsets.push_back({
                std::cos(currentAngle) * radius,
                std::sin(currentAngle) * radius
            });
        }
        return offsets;
    }
};

#endif // STRUCTURE_DEFINITION_HPP
