#ifndef CHEMISTRY_DATABASE_HPP
#define CHEMISTRY_DATABASE_HPP

#include "Element.hpp"
#include "Molecule.hpp"
#include <vector>
#include <string>
#include <unordered_map>

/**
 * CHEMISTRY DATABASE (Optimized V2)
 * Registers and provides O(1) access to all elements.
 * Uses a vector indexed by atomic number for maximum performance.
 */
class ChemistryDatabase {
public:
    static ChemistryDatabase& getInstance() {
        static ChemistryDatabase instance;
        return instance;
    }

    void initialize() { reload(); }
    void reload();

    // Get an element by its atomic number (O(1) Direct Access)
    const Element& getElement(int atomicNumber) const;
    
    // Get an element by its symbol (O(1) Hash Map)
    const Element& getElement(const std::string& symbol) const;

    bool exists(int atomicNumber) const;
    
    // Molecule Management
    const Molecule* findMoleculeByComposition(const std::map<int, int>& composition) const;
    const std::vector<Molecule>& getAllMolecules() const { return molecules; }

    // Discovery & Spawning
    std::vector<int> getSpawnableAtomicNumbers() const;
    int getRandomSpawnableAtomicNumber() const;

    // Get list of all valid IDs for the Quimidex
    std::vector<int> getRegisteredAtomicNumbers() const {
        std::vector<int> ids;
        for (int i = 0; i < (int)elements.size(); i++) {
            if (elements[i].atomicNumber != 0) ids.push_back(i);
        }
        return ids;
    }

private:
    ChemistryDatabase(); // Initializes elements
    
    // Dense storage for ID-based access
    std::vector<Element> elements; 
    std::vector<Molecule> molecules;
    
    // Fast lookup map by Symbol
    std::unordered_map<std::string, int> symbolToId;
    
    void addElement(Element e);
    void addMolecule(Molecule m);
    
    // VALIDATION: Ensures all elements have proper 2.5D Z-axis variance
    void validateElements() const;
};

#endif
