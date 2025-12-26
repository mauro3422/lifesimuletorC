#include "ChemistryDatabase.hpp"
#include "../core/JsonLoader.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

ChemistryDatabase::ChemistryDatabase() {
    elements.resize(120); 

    // Load elements from JSON file
    try {
        std::vector<Element> loadedElements = JsonLoader::loadElements("data/elements.json");
        for (const Element& el : loadedElements) {
            addElement(el);
        }
        TraceLog(LOG_INFO, "[CHEMISTRY] Loaded %d elements from JSON", (int)loadedElements.size());
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[CHEMISTRY] Failed to load elements.json: %s", e.what());
        TraceLog(LOG_ERROR, "[CHEMISTRY] Game cannot start without valid chemistry data!");
        throw; // Re-throw to prevent game from starting with invalid data
    }

    // --- REGISTRO DE MOLÉCULAS ---
    addMolecule({
        "H2", "Hidrógeno Diatómico", "H2", "PRIMORDIAL",
        "La forma más simple de molécula covalente.",
        "Combustible estelar y precursor de toda la química compleja.",
        "Big Bang y nubes moleculares frías.",
        SKYBLUE,
        {{1, 2}}
    });

    addMolecule({
        "H2O", "Agua", "H2O", "VITAL",
        "La molécula de la vida. Única sustancia que existe naturalmente en los 3 estados en la Tierra.",
        "Solvente universal donde ocurre toda la bioquímica celular.",
        "Fusión de carbono y helio en estrellas masivas.",
        BLUE,
        {{1, 2}, {8, 1}}
    });

    // --- NUEVAS MOLÉCULAS ---
    addMolecule({
        "O2", "Oxígeno Diatómico", "O2", "ATMOSFÉRICO",
        "Gas incoloro e inodoro, esencial para la respiración aeróbica.",
        "Aceptor final de electrones en la cadena de transporte molecular.",
        "Nucleosíntesis estelar y procesos fotosintéticos posteriores.",
        RED,
        {{8, 2}}
    });

    addMolecule({
        "N2", "Nitrógeno Diatómico", "N2", "INERTE",
        "Gas triple enlace extremadamente estable. Constituye la mayor parte del aire.",
        "Provee una atmósfera estable y es el reservorio de nitrógeno biótico.",
        "Ciclo CNO en gigantes rojas.",
        BLUE,
        {{7, 2}}
    });

    addMolecule({
        "CH4", "Metano", "CH4", "ORGÁNICO",
        "El hidrocarburo más simple. Un potente gas de efecto invernadero.",
        "Precursor fundamental en la síntesis de moléculas orgánicas complejas.",
        "Procesos geológicos hidrotermales y nebulosas planetarias.",
        GREEN,
        {{6, 1}, {1, 4}}
    });

    addMolecule({
        "NH3", "Amoníaco", "NH3", "PRECURSOR",
        "Compuesto de nitrógeno e hidrógeno con un olor penetrante característico.",
        "Fuente crítica de nitrógeno para la síntesis de aminoácidos primordiales.",
        "Nubes de gas interestelar y volcanismo temprano.",
        VIOLET,
        {{7, 1}, {1, 3}}
    });

    addMolecule({
        "CO2", "Dióxido de Carbono", "CO2", "VOLÁTIL",
        "Compuesto lineal que regula el clima planetario mediante el efecto invernadero.",
        "Fuente de carbono para la fijación autótrofa y precursor de azúcares.",
        "Desgasificación volcánica y oxidación de materia orgánica.",
        LIGHTGRAY,
        {{6, 1}, {8, 2}}
    });
    
    // MANDATORY VALIDATION: Ensure all elements have 2.5D Z-axis variance
    validateElements();
}

void ChemistryDatabase::addMolecule(Molecule m) {
    molecules.push_back(m);
}

const Molecule* ChemistryDatabase::findMoleculeByComposition(const std::map<int, int>& composition) const {
    for (const auto& mol : molecules) {
        if (mol.composition == composition) return &mol;
    }
    return nullptr;
}

void ChemistryDatabase::addElement(Element e) {
    if (e.atomicNumber < 0 || e.atomicNumber >= (int)elements.size()) {
        elements.resize(e.atomicNumber + 10);
    }
    elements[e.atomicNumber] = e;
    symbolToId[e.symbol] = e.atomicNumber;
}

bool ChemistryDatabase::exists(int atomicNumber) const {
    if (atomicNumber <= 0 || atomicNumber >= (int)elements.size()) return false;
    return elements[atomicNumber].atomicNumber != 0;
}

const Element& ChemistryDatabase::getElement(int atomicNumber) const {
    if (atomicNumber <= 0 || atomicNumber >= (int)elements.size() || elements[atomicNumber].atomicNumber == 0) {
        throw std::runtime_error("Elemento no encontrado en la base de datos");
    }
    return elements[atomicNumber];
}

const Element& ChemistryDatabase::getElement(const std::string& symbol) const {
    auto it = symbolToId.find(symbol);
    if (it != symbolToId.end()) return getElement(it->second);
    throw std::runtime_error("Símbolo químico no registrado");
}

// VALIDATION: Ensures all elements have proper 2.5D Z-axis variance in bondingSlots
void ChemistryDatabase::validateElements() const {
    for (int i = 1; i < (int)elements.size(); i++) {
        const Element& el = elements[i];
        if (el.atomicNumber == 0) continue; // Skip empty slots
        
        // Skip H (only 1 bond, no angle needed)
        if (el.maxBonds <= 1) continue;
        
        // Check if bondingSlots have Z variance
        if (el.bondingSlots.size() < 2) continue;
        
        bool hasZVariance = false;
        float firstZ = el.bondingSlots[0].z;
        
        for (size_t j = 1; j < el.bondingSlots.size(); j++) {
            if (std::abs(el.bondingSlots[j].z - firstZ) > 0.05f) {
                hasZVariance = true;
                break;
            }
        }
        
        if (!hasZVariance) {
            TraceLog(LOG_ERROR, "[CHEMISTRY VALIDATION FAILED]");
            TraceLog(LOG_ERROR, "Element %s (Z=%d) has NO Z-axis variance in bondingSlots!", 
                     el.symbol.c_str(), el.atomicNumber);
            TraceLog(LOG_ERROR, "This will cause visual overlap in 2.5D mode.");
            TraceLog(LOG_ERROR, "FIX: Add Z offset to bondingSlots, e.g. norm({x, y, 0.3f})");
            throw std::runtime_error(
                "Element " + el.symbol + " missing Z-axis variance in bondingSlots. "
                "All elements with >1 bond must have Z variance for 2.5D visualization."
            );
        }
    }
    TraceLog(LOG_INFO, "[CHEMISTRY] All elements passed 2.5D Z-axis validation ✓");
}
