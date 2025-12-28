#ifndef ELEMENT_HPP
#define ELEMENT_HPP

#include "raylib.h"
#include <string>
#include <vector>

/**
 * REPRESENTACIÓN DE UN ELEMENTO QUÍMICO
 * Contiene datos físicos, lore y estética para la UI.
 */
struct Element {
    int atomicNumber;
    std::string symbol;
    std::string name;
    float atomicMass;
    float vdWRadius;  // Van der Waals radius (in picometers or game units)
    Color color;
    Color backgroundColor;  // Background color for UI element cards
    
    // Datos para el Quimidex / Lore
    std::string category;     // Ej: "No metal", "Gas noble"
    std::string description;  // Scientific/fantasy lore
    std::string origin;       // Origin/Where found (Stars, Nucleosynthesis, etc)
    std::string discoveryHint; // How to find it in the game
    
    // Valence Configuration (for deterministic bonds)
    int maxBonds;
    float electronegativity;
    std::vector<Vector3> bondingSlots; // Vectores directores VSEPR
};

#endif
