#ifndef JSON_LOADER_HPP
#define JSON_LOADER_HPP

#include "json.hpp"
#include "../chemistry/Element.hpp"
#include "../chemistry/Molecule.hpp"
#include "raylib.h"
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

using json = nlohmann::json;

namespace JsonLoader {

    // Utility: Normalize a vector
    inline Vector3 normalize(Vector3 v) {
        float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        if (len < 0.0001f) return {0, 0, 0};
        return {v.x/len, v.y/len, v.z/len};
    }

    // Parse Color from JSON
    inline Color parseColor(const json& j) {
        return {
            static_cast<unsigned char>(j.value("r", 255)),
            static_cast<unsigned char>(j.value("g", 255)),
            static_cast<unsigned char>(j.value("b", 255)),
            static_cast<unsigned char>(j.value("a", 255))
        };
    }

    // Parse Vector3 from JSON
    inline Vector3 parseVector3(const json& j) {
        return {
            j.value("x", 0.0f),
            j.value("y", 0.0f),
            j.value("z", 0.0f)
        };
    }

    // Validate Element - throws if invalid
    inline void validateElement(const Element& el) {
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

    // Load Elements from JSON file
    inline std::vector<Element> loadElements(const std::string& path) {
        std::vector<Element> elements;
        
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("[JSON LOADER] Cannot open file: " + path);
        }
        
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
            el.name = j.value("name", "Unknown");
            el.atomicMass = j.value("atomicMass", 0.0f);
            el.vdWRadius = j.value("vdWRadius", 1.5f);
            el.maxBonds = j.value("maxBonds", 1);
            el.electronegativity = j.value("electronegativity", 2.0f);
            
            // Colors
            if (j.contains("color")) {
                el.color = parseColor(j["color"]);
            } else {
                el.color = GRAY;
            }
            
            if (j.contains("backgroundColor")) {
                el.backgroundColor = parseColor(j["backgroundColor"]);
            } else {
                // Default: darker version of color
                el.backgroundColor = {
                    static_cast<unsigned char>(el.color.r / 4),
                    static_cast<unsigned char>(el.color.g / 4),
                    static_cast<unsigned char>(el.color.b / 4),
                    255
                };
            }
            
            // Lore/UI data
            el.category = j.value("category", "Unknown");
            el.description = j.value("description", "");
            el.origin = j.value("origin", "");
            el.discoveryHint = j.value("discoveryHint", "");
            
            // Bonding Slots
            if (j.contains("bondingSlots") && j["bondingSlots"].is_array()) {
                for (const auto& slot : j["bondingSlots"]) {
                    Vector3 v = parseVector3(slot);
                    el.bondingSlots.push_back(normalize(v));
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

} // namespace JsonLoader

#endif // JSON_LOADER_HPP
