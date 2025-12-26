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
    float vdWRadius;  // Radio de Van der Waals (en picómetros o unidades de juego)
    Color color;
    Color backgroundColor;  // Background color for UI element cards
    
    // Datos para el Quimidex / Lore
    std::string category;     // Ej: "No metal", "Gas noble"
    std::string description;  // Lore científico/fantástico
    std::string origin;       // Origen/Donde se encuentra (Estrellas, Nucleosíntesis, etc)
    std::string discoveryHint; // Cómo encontrarlo en el juego
    
    // Configuración de Valencias (para enlaces deterministas)
    int maxBonds;
    float electronegativity;
    std::vector<Vector3> bondingSlots; // Vectores directores VSEPR
};

#endif
