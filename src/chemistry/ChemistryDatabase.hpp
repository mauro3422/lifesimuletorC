#ifndef CHEMISTRY_DATABASE_HPP
#define CHEMISTRY_DATABASE_HPP

#include "Element.hpp"
#include "Molecule.hpp"
#include <vector>
#include <string>
#include <unordered_map>

/**
 * BASE DE DATOS DE QUÍMICA (Optimized V2)
 * Registra y provee acceso O(1) a todos los elementos.
 * Utiliza un vector indexado por número atómico para máximo rendimiento.
 */
class ChemistryDatabase {
public:
    static ChemistryDatabase& getInstance() {
        static ChemistryDatabase instance;
        return instance;
    }

    void initialize() { reload(); }
    void reload();

    // Obtener un elemento por su número atómico (O(1) Direct Access)
    const Element& getElement(int atomicNumber) const;
    
    // Obtener un elemento por su símbolo (O(1) Hash Map)
    const Element& getElement(const std::string& symbol) const;

    bool exists(int atomicNumber) const;
    
    // Gestión de Moléculas
    const Molecule* findMoleculeByComposition(const std::map<int, int>& composition) const;
    const std::vector<Molecule>& getAllMolecules() const { return molecules; }

    // Discovery & Spawning
    std::vector<int> getSpawnableAtomicNumbers() const;
    int getRandomSpawnableAtomicNumber() const;

    // Obtener lista de todos los IDs válidos para el Quimidex
    std::vector<int> getRegisteredAtomicNumbers() const {
        std::vector<int> ids;
        for (int i = 0; i < (int)elements.size(); i++) {
            if (elements[i].atomicNumber != 0) ids.push_back(i);
        }
        return ids;
    }

private:
    ChemistryDatabase(); // Inicializa los elementos
    
    // Almacenamiento denso para acceso por ID
    std::vector<Element> elements; 
    std::vector<Molecule> molecules;
    
    // Mapa rápido para búsqueda por Símbolo
    std::unordered_map<std::string, int> symbolToId;
    
    void addElement(Element e);
    void addMolecule(Molecule m);
    
    // VALIDATION: Ensures all elements have proper 2.5D Z-axis variance
    void validateElements() const;
};

#endif
