#include "JsonLoader.hpp"
#include "MathUtils.hpp"
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace JsonLoader {

    // Parse Color from JSON
    Color parseColor(const json& j) {
        return {
            static_cast<unsigned char>(j.value("r", 255)),
            static_cast<unsigned char>(j.value("g", 255)),
            static_cast<unsigned char>(j.value("b", 255)),
            static_cast<unsigned char>(j.value("a", 255))
        };
    }

    // Parse Vector3 from JSON
    Vector3 parseVector3(const json& j) {
        return {
            j.value("x", 0.0f),
            j.value("y", 0.0f),
            j.value("z", 0.0f)
        };
    }

    // Validate Element - throws if invalid
    void validateElement(const Element& el) {
        std::string errors;
        
        // Required field checks
        if (el.atomicNumber <= 0) 
            errors += "atomicNumber must be > 0. ";
        if (el.symbol.empty() || el.symbol.length() > 3) 
            errors += "symbol must be 1-3 characters. ";
        if (el.name.empty()) 
            errors += "name cannot be empty. ";
        if (el.maxBonds < 1 || el.maxBonds > 8) 
            errors += "maxBonds must be 1-8. ";
        if (el.electronegativity < 0.5f || el.electronegativity > 4.0f) 
            errors += "electronegativity must be 0.5-4.0. ";
        
        // Bonding slots validation
        if ((int)el.bondingSlots.size() != el.maxBonds) {
            errors += "bondingSlots.size() must equal maxBonds. ";
        }
        
        // Z-axis variance check (if maxBonds > 1)
        if (el.maxBonds > 1 && el.bondingSlots.size() >= 2) {
            bool hasZVariance = false;
            float firstZ = el.bondingSlots[0].z;
            for (size_t i = 1; i < el.bondingSlots.size(); i++) {
                if (std::abs(el.bondingSlots[i].z - firstZ) > 0.05f) {
                    hasZVariance = true;
                    break;
                }
            }
            if (!hasZVariance) {
                errors += "bondingSlots need Z-axis variance for 2.5D visualization. ";
            }
        }
        
        // Color validation (non-zero alpha)
        if (el.color.a == 0) 
            errors += "color.a (alpha) cannot be 0. ";
        if (el.backgroundColor.a == 0) 
            errors += "backgroundColor.a (alpha) cannot be 0. ";
        
        if (!errors.empty()) {
            throw std::runtime_error(
                "[CHEMISTRY VALIDATION] Element " + el.symbol + 
                " (Z=" + std::to_string(el.atomicNumber) + ") failed: " + errors
            );
        }
    }

    // Load Elements from JSON file (Localized)
    std::vector<Element> loadElements(const std::string& path, const std::string& lang) {
        std::vector<Element> elements;
        std::ifstream file(path);
        if (!file.is_open()) throw std::runtime_error("[JSON LOADER] Cannot open: " + path);
        
        json data;
        try {
            file >> data;
        } catch (const json::parse_error& e) {
            throw std::runtime_error("[JSON LOADER] Parse error in " + path + ": " + e.what());
        }
        
        if (!data.contains("elements") || !data["elements"].is_array()) {
            throw std::runtime_error("[JSON LOADER] Missing 'elements' array in " + path);
        }
        
        for (const auto& j : data["elements"]) {
            Element el;
            
            // Required fields
            el.atomicNumber = j.value("atomicNumber", 0);
            el.symbol = j.value("symbol", "");
            el.atomicMass = j.value("atomicMass", 0.0f);
            el.vdWRadius = j.value("vdWRadius", 1.5f);
            el.maxBonds = j.value("maxBonds", 1);
            el.electronegativity = j.value("electronegativity", 2.0f);
            
            // Localized Fields
            if (j.contains("name") && j["name"].is_object() && j["name"].contains(lang)) 
                el.name = j["name"][lang].get<std::string>();
            else if (j.contains("name") && j["name"].is_object())
                el.name = j["name"].value("en", "Unknown");
            else
                el.name = j.value("name", "Unknown");

            if (j.contains("category") && j["category"].is_object() && j["category"].contains(lang)) 
                el.category = j["category"][lang].get<std::string>();
            else if (j.contains("category") && j["category"].is_object())
                el.category = j["category"].value("en", "Unknown");
            else
                el.category = j.value("category", "Unknown");

            if (j.contains("description") && j["description"].is_object() && j["description"].contains(lang)) 
                el.description = j["description"][lang].get<std::string>();
            else if (j.contains("description") && j["description"].is_object())
                el.description = j["description"].value("en", "");
            else
                el.description = j.value("description", "");

            if (j.contains("origin") && j["origin"].is_object() && j["origin"].contains(lang)) 
                el.origin = j["origin"][lang].get<std::string>();
            else if (j.contains("origin") && j["origin"].is_object())
                el.origin = j["origin"].value("en", "");
            else
                el.origin = j.value("origin", "");

            if (j.contains("discoveryHint") && j["discoveryHint"].is_object() && j["discoveryHint"].contains(lang)) 
                el.discoveryHint = j["discoveryHint"][lang].get<std::string>();
            else if (j.contains("discoveryHint") && j["discoveryHint"].is_object())
                el.discoveryHint = j["discoveryHint"].value("en", "");
            else
                el.discoveryHint = j.value("discoveryHint", "");

            // Colors
            if (j.contains("color")) {
                el.color = parseColor(j["color"]);
            } else {
                el.color = GRAY;
            }
            
            if (j.contains("backgroundColor")) {
                el.backgroundColor = parseColor(j["backgroundColor"]);
            } else {
                el.backgroundColor = {
                    static_cast<unsigned char>(el.color.r / 4),
                    static_cast<unsigned char>(el.color.g / 4),
                    static_cast<unsigned char>(el.color.b / 4),
                    255
                };
            }
            
            // Bonding Slots
            if (j.contains("bondingSlots") && j["bondingSlots"].is_array()) {
                for (const auto& slot : j["bondingSlots"]) {
                    Vector3 v = parseVector3(slot);
                    el.bondingSlots.push_back(MathUtils::normalize(v));
                }
            }
            
            // Validate and add
            validateElement(el);
            elements.push_back(el);
            
            TraceLog(LOG_INFO, "[JSON LOADER] Loaded element: %s (Z=%d)", 
                     el.symbol.c_str(), el.atomicNumber);
        }
        
        TraceLog(LOG_INFO, "[JSON LOADER] Successfully loaded %d elements from %s", 
                 (int)elements.size(), path.c_str());
        
        return elements;
    }

    // Load Missions from JSON file (Localized)
    std::vector<Mission> loadMissions(const std::string& path, const std::string& lang) {
        std::vector<Mission> missions;
        std::ifstream file(path);
        if (!file.is_open()) throw std::runtime_error("[JSON LOADER] Cannot open missions: " + path);
        
        json data;
        file >> data;
        
        for (const auto& j : data) {
            Mission m;
            m.id = j.value("id", "unknown");
            m.reward = j.value("reward", "");
            m.tier = j.value("tier", 0);
            m.status = MissionStatus::AVAILABLE; // Default

            // Localized fields
            if (j.contains("title") && j["title"].contains(lang)) 
                m.title = j["title"][lang].get<std::string>();
            else m.title = j["title"].value("en", "Untitled");

            if (j.contains("description") && j["description"].contains(lang)) 
                m.description = j["description"][lang].get<std::string>();
            else m.description = j["description"].value("en", "");

            if (j.contains("scientificContext") && j["scientificContext"].contains(lang)) 
                m.scientificContext = j["scientificContext"][lang].get<std::string>();
            else m.scientificContext = j["scientificContext"].value("en", "");

            missions.push_back(m);
        }
        return missions;
    }

    // Load Molecules from JSON file (Localized)
    std::vector<Molecule> loadMolecules(const std::string& path, const std::string& lang) {
        std::vector<Molecule> molecules;
        std::ifstream file(path);
        if (!file.is_open()) throw std::runtime_error("[JSON LOADER] Cannot open molecules: " + path);
        
        json data;
        file >> data;
        
        for (const auto& j : data) {
            Molecule m;
            m.id = j.value("id", "unknown");
            m.formula = j.value("formula", "");
            m.category = j.value("category", "");
            m.color = parseColor(j["color"]);

            // Composition: Map<StringId, Int> -> Map<AtomicNumber, Int>
            if (j.contains("composition") && j["composition"].is_object()) {
                for (auto& [key, val] : j["composition"].items()) {
                    m.composition[std::stoi(key)] = val.get<int>();
                }
            }

            // Localized fields
            if (j.contains("name") && j["name"].contains(lang)) 
                m.name = j["name"][lang].get<std::string>();
            else m.name = j["name"].value("en", "Unnamed Molecule");

            if (j.contains("description") && j["description"].contains(lang)) 
                m.description = j["description"][lang].get<std::string>();
            else m.description = j["description"].value("en", "");

            if (j.contains("biologicalSignificance") && j["biologicalSignificance"].contains(lang)) 
                m.biologicalSignificance = j["biologicalSignificance"][lang].get<std::string>();
            else m.biologicalSignificance = j["biologicalSignificance"].value("en", "");

            if (j.contains("origin") && j["origin"].contains(lang)) 
                m.origin = j["origin"][lang].get<std::string>();
            else m.origin = j["origin"].value("en", "");

            molecules.push_back(m);
        }
        return molecules;
    }

    // Load Structures from JSON file
    std::vector<StructureDefinition> loadStructures(const std::string& path) {
        std::vector<StructureDefinition> structures;
        std::ifstream file(path);
        if (!file.is_open()) throw std::runtime_error("[JSON LOADER] Cannot open structures: " + path);
        
        json data;
        try {
            file >> data;
        } catch (const json::parse_error& e) {
            throw std::runtime_error("[JSON LOADER] Parse error in " + path + ": " + e.what());
        }
        
        if (!data.contains("structures") || !data["structures"].is_array()) {
            throw std::runtime_error("[JSON LOADER] Missing 'structures' array in " + path);
        }

        for (const auto& j : data["structures"]) {
            StructureDefinition s;
            s.name = j.value("name", "unknown");
            s.atomCount = j.value("atomCount", 0);
            s.atomicNumber = j.value("atomicNumber", 0);
            s.targetAngle = j.value("targetAngle", 1.5708f); // Default 90 deg
            s.damping = j.value("damping", 0.30f);
            s.globalDamping = j.value("globalDamping", 0.98f);
            s.formationSpeed = j.value("formationSpeed", 2.0f);
            s.formationDamping = j.value("formationDamping", 0.90f);
            s.maxFormationSpeed = j.value("maxFormationSpeed", 400.0f);
            s.completionThreshold = j.value("completionThreshold", 0.8f);
            s.rotationOffset = j.value("rotationOffset", 0.0f);
            s.isPlanar = j.value("isPlanar", true);
            s.instantFormation = j.value("instantFormation", true);
            
            structures.push_back(s);
            TraceLog(LOG_INFO, "[JSON LOADER] Loaded structure definition: %s", s.name.c_str());
        }
        
        return structures;
    }

} // namespace JsonLoader
