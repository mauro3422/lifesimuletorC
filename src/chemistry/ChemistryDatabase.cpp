#include "ChemistryDatabase.hpp"
#include "../core/JsonLoader.hpp"
#include "../core/LocalizationManager.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

ChemistryDatabase::ChemistryDatabase() {
    elements.resize(120); 
    // Initial load, if it fails, the constructor should fail.
    // The reload method handles subsequent reloads which might be more forgiving.
    try {
        reload(); 
    } catch (const std::exception& e) {
        // Re-throw to indicate construction failure
        throw;
    }
}

void ChemistryDatabase::reload() {
    std::string lang = LocalizationManager::getInstance().getLanguageCode();
    
    // 1. Clear current state (Molecules only, elements stay in slots but get updated)
    molecules.clear();
    symbolToId.clear(); // Clear symbol-to-ID mapping as elements will be re-added

    // 2. Load Elements from JSON (Localized)
    // Note: elements vector itself is not cleared, but elements are overwritten by addElement.
    // This allows elements to retain their atomicNumber-based indices.
    // If an element is removed from JSON, its old data might persist until overwritten.
    // For a full clear, elements would need to be re-initialized or cleared.
    // For now, we assume new elements will overwrite old ones or fill empty slots.
    // To ensure a clean slate for elements, we could re-initialize them:
    // for (int i = 0; i < elements.size(); ++i) elements[i] = Element(); // Reset all elements
    
    try {
        std::vector<Element> loadedElements = JsonLoader::loadElements("data/elements.json", lang);
        for (const Element& el : loadedElements) {
            addElement(el);
        }
        TraceLog(LOG_INFO, "[CHEMISTRY] Reloaded %d elements from JSON (Language: %s)", (int)loadedElements.size(), lang.c_str());
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[CHEMISTRY] Failed to reload elements.json: %s", e.what());
        // For initial construction, this exception is re-thrown by the constructor.
        // For subsequent reloads, we might just log and continue with potentially incomplete data.
        // However, element data is critical, so re-throwing here ensures consistency.
        throw; 
    }

    // 3. Load Localized Molecules from JSON
    try {
        molecules = JsonLoader::loadMolecules("data/molecules.json", lang);
        TraceLog(LOG_INFO, "[CHEMISTRY] Reloaded %d molecules from JSON (Language: %s)", (int)molecules.size(), lang.c_str());
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[CHEMISTRY] Failed to reload molecules.json: %s", e.what());
        // Molecule loading failure is less critical than element loading, so we just log.
    }

    // MANDATORY VALIDATION
    validateElements(); // This method throws if validation fails.
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
        throw std::runtime_error("Element not found in database");
    }
    return elements[atomicNumber];
}

const Element& ChemistryDatabase::getElement(const std::string& symbol) const {
    auto it = symbolToId.find(symbol);
    if (it != symbolToId.end()) return getElement(it->second);
    throw std::runtime_error("Chemical symbol not registered");
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

std::vector<int> ChemistryDatabase::getSpawnableAtomicNumbers() const {
    std::vector<int> spawnable;
    for (const auto& el : elements) {
        if (el.atomicNumber != 0) {
            spawnable.push_back(el.atomicNumber);
        }
    }
    return spawnable;
}

int ChemistryDatabase::getRandomSpawnableAtomicNumber() const {
    auto spawnable = getSpawnableAtomicNumbers();
    if (spawnable.empty()) return 1; // Hydrogen fallback
    return spawnable[GetRandomValue(0, (int)spawnable.size() - 1)];
}

