#ifndef JSON_LOADER_HPP
#define JSON_LOADER_HPP

#include "json.hpp"
#include "../chemistry/Element.hpp"
#include "../chemistry/Molecule.hpp"
#include "../chemistry/StructureDefinition.hpp"
#include "../gameplay/MissionManager.hpp"
#include "raylib.h"
#include <vector>
#include <string>

using json = nlohmann::json;

namespace JsonLoader {

    // Parse Color from JSON
    Color parseColor(const json& j);

    // Parse Vector3 from JSON
    Vector3 parseVector3(const json& j);

    // Validate Element - throws if invalid
    void validateElement(const Element& el);

    // Load Elements from JSON file (Localized)
    std::vector<Element> loadElements(const std::string& path, const std::string& lang = "es");

    // Load Missions from JSON file (Localized)
    std::vector<Mission> loadMissions(const std::string& path, const std::string& lang = "es");

    // Load Molecules from JSON file (Localized)
    std::vector<Molecule> loadMolecules(const std::string& path, const std::string& lang = "es");

    // Load Structures from JSON file
    std::vector<struct StructureDefinition> loadStructures(const std::string& path);

} // namespace JsonLoader

#endif // JSON_LOADER_HPP
