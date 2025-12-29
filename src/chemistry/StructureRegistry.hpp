#ifndef STRUCTURE_REGISTRY_HPP
#define STRUCTURE_REGISTRY_HPP

#include <vector>
#include <string>
#include "StructureDefinition.hpp"

class StructureRegistry {
public:
    static StructureRegistry& getInstance();

    // Loads definitions from JSON
    void loadFromDisk(const std::string& path);

    // Manual registration (backup/debug)
    void registerStructure(const StructureDefinition& def);

    // Finds a match based on atom count and atomic number
    // returns nullptr if no match found
    const StructureDefinition* findMatch(int atomCount, int atomicNumber) const;

    // Checks if an atom is part of a specific structure (by ID and type)
    // For now, we mainly use findMatch for formation.
    
    const std::vector<StructureDefinition>& getAllStructures() const { return structures; }
    
    // Override instantFormation for all structures (for testing/animation mode)
    void setInstantFormation(bool instant) {
        for (auto& s : structures) {
            s.instantFormation = instant;
        }
    }

private:
    StructureRegistry() = default;
    std::vector<StructureDefinition> structures;

    // Disable copy
    StructureRegistry(const StructureRegistry&) = delete;
    StructureRegistry& operator=(const StructureRegistry&) = delete;
};

#endif // STRUCTURE_REGISTRY_HPP
