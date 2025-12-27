#include "StructureRegistry.hpp"
#include "../core/JsonLoader.hpp"
#include "raylib.h"

StructureRegistry& StructureRegistry::getInstance() {
    static StructureRegistry instance;
    return instance;
}

void StructureRegistry::loadFromDisk(const std::string& path) {
    try {
        structures = JsonLoader::loadStructures(path);
        TraceLog(LOG_INFO, "[STRUCTURES] Loaded %d structure definitions from %s", (int)structures.size(), path.c_str());
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[STRUCTURES] Failed to load %s: %s", path.c_str(), e.what());
    }
}

void StructureRegistry::registerStructure(const StructureDefinition& def) {
    structures.push_back(def);
}

const StructureDefinition* StructureRegistry::findMatch(int atomCount, int atomicNumber) const {
    for (const auto& s : structures) {
        // atomicNumber 0 means any element matches
        bool elementMatches = (s.atomicNumber == 0 || s.atomicNumber == atomicNumber);
        if (s.atomCount == atomCount && elementMatches) {
            return &s;
        }
    }
    return nullptr;
}
