#ifndef MOLECULE_HPP
#define MOLECULE_HPP

#include "raylib.h"
#include <string>
#include <vector>
#include <map>

struct Molecule {
    std::string id;          // Ej: "H2O"
    std::string name;        // Ej: "Agua"
    std::string formula;     // Ej: "H2O1"
    std::string category;    // E.g.: "Vital", "Prebiotic"
    std::string description; // Lore
    std::string biologicalSignificance; // Use in biology
    std::string origin;      // Procedencia
    Color color;             // Color representativo para la UI
    
    std::map<int, int> composition; // Map<AtomicNumber, Count> (e.g. {1: 2, 8: 1} for H2O)
};

#endif
