#include "ChemistryDatabase.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

ChemistryDatabase::ChemistryDatabase() {
    elements.resize(120); 

    auto norm = [](Vector3 v) {
        float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        return (Vector3){ v.x/len, v.y/len, v.z/len };
    };

    // HIDRÓGENO
    addElement({
        1, "H", "Hidrógeno", 1.008f, 1.2f, SKYBLUE,
        "No metal", 
        "El elemento más abundante del universo. Ligero y reactivo.",
        "Nucleosíntesis estelar primordial desatada tras el Big Bang.", // ORIGIN prolongado para testear wrap
        "Se encuentra en nubes de gas primordiales y estrellas jóvenes.",
        1, 2.20f,
        { {1, 0, 0} }
    });

    // CARBONO
    addElement({
        6, "C", "Carbono", 12.011f, 1.7f, DARKGRAY,
        "No metal", 
        "La base de la vida orgánica. Capaz de formar complejas cadenas infinitas y estructuras estables.",
        "Gigantes rojas a través del proceso triple alfa.", 
        "Aparece al procesar materia orgánica densa o grafito.",
        4, 2.55f,
        { 
            norm({1, 1, 1}), 
            norm({1, -1, -1}), 
            norm({-1, 1, -1}), 
            norm({-1, -1, 1}) 
        }
    });

    // NITROGENO
    addElement({
        7, "N", "Nitrógeno", 14.007f, 1.55f, BLUE,
        "No metal", 
        "Vital en aminoácidos y bases nitrogenadas. Inerte como gas libre.",
        "Ciclo CNO en el corazón de estrellas masivas.", 
        "Detección común en atmósferas estables de mundos templados.",
        3, 3.04f,
        {
            norm({1, 0, -0.5f}),
            norm({-0.5f, 0.866f, -0.5f}),
            norm({-0.5f, -0.866f, -0.5f})
        }
    });

    // OXÍGENO
    addElement({
        8, "O", "Oxígeno", 15.999f, 1.52f, RED,
        "No metal", 
        "Oxidante potente necesario para la respiración celular y combustión.",
        "Fusión de helio y neón en capas estelares masivas.", 
        "Presente en silicatos, agua y atmósfera respirable.",
        2, 3.44f,
        {
            norm({1, 0.7f, 0}),
            norm({1, -0.7f, 0})
        }
    });

    // FÓSFORO (Nuevo!)
    addElement({
        15, "P", "Fósforo", 30.974f, 1.8f, ORANGE,
        "No metal",
        "Crucial para el ADN y la transferencia de energía celular (ATP).",
        "Nucleosíntesis en supernovas de Tipo II y estrellas masivas.",
        "Raro en estado puro, común en depósitos minerales antiguos.",
        5, 2.19f,
        {
            norm({1, 0, 0}), norm({-0.5f, 0.866f, 0}), norm({-0.5f, -0.866f, 0}), // Ecuatoriales
            norm({0, 0, 1}), norm({0, 0, -1}) // Axiales
        }
    });

    // AZUFRE (Nuevo!)
    addElement({
        16, "S", "Azufre", 32.065f, 1.8f, YELLOW,
        "No metal",
        "Participa en puentes disulfuro proteicos. Olor característico en compuestos.",
        "Fusión de silicio en núcleos de estrellas masivas pre-supernova.",
        "Abundante en regiones de actividad volcánica tectónica.",
        2, 2.58f,
        {
            norm({1, 0.3f, 0}),
            norm({1, -0.3f, 0})
        }
    });
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
