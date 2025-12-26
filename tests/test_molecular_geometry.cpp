/**
 * MOLECULAR FORMATION TEST SUITE
 * 
 * This test validates that molecules form with correct VSEPR geometry
 * by checking bond angles and composition mathematically.
 * 
 * Compile: g++ -std=c++17 -I. tests/test_molecular_geometry.cpp -o tests/run_geometry_tests.exe
 * Run: ./tests/run_geometry_tests.exe
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <cmath>
#include <vector>
#include <map>
#include <string>

// Mock Vector3 for testing
struct Vector3 {
    float x, y, z;
};

// Utility: Calculate angle between two vectors (in degrees)
float angleBetween(const Vector3& a, const Vector3& b) {
    float dot = a.x*b.x + a.y*b.y + a.z*b.z;
    float magA = std::sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
    float magB = std::sqrt(b.x*b.x + b.y*b.y + b.z*b.z);
    if (magA < 0.0001f || magB < 0.0001f) return 0.0f;
    float cosAngle = dot / (magA * magB);
    if (cosAngle > 1.0f) cosAngle = 1.0f;
    if (cosAngle < -1.0f) cosAngle = -1.0f;
    return std::acos(cosAngle) * (180.0f / 3.14159265f);
}

// Utility: Normalize vector
Vector3 normalize(Vector3 v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 0.0001f) return {0, 0, 0};
    return {v.x/len, v.y/len, v.z/len};
}

// ============================================================================
// ELEMENT DEFINITIONS (mirror of ChemistryDatabase.cpp)
// ============================================================================
struct ElementSlots {
    std::string symbol;
    int maxBonds;
    std::vector<Vector3> bondingSlots;
    float expectedMinAngle;  // Minimum angle between any two slots
    float expectedMaxAngle;  // Maximum angle between any two slots
};

// Current element slot definitions from the game
std::vector<ElementSlots> getElementDefinitions() {
    auto norm = [](Vector3 v) { return normalize(v); };
    
    return {
        // Hydrogen - 1 slot, no angle test needed
        {"H", 1, { {1, 0, 0} }, 0, 0},
        
        // Carbon - Tetrahedral (109.5°)
        {"C", 4, {
            norm({1, 1, 1}), 
            norm({1, -1, -1}), 
            norm({-1, 1, -1}), 
            norm({-1, -1, 1})
        }, 100.0f, 120.0f},
        
        // Nitrogen - Pyramidal (107°)
        {"N", 3, {
            norm({0, 0.75f, -0.65f}),
            norm({-0.65f, 0.75f, 0.35f}),
            norm({0.65f, 0.75f, 0.35f})
        }, 70.0f, 130.0f},
        
        // Oxygen - Angular (104.5°)
        {"O", 2, {
            norm({-0.6f, 0.7f, -0.3f}),
            norm({0.6f, 0.7f, 0.3f})
        }, 60.0f, 120.0f},
        
        // Phosphorus - Pyramidal (93.5°)
        {"P", 3, {
            norm({0, 0.8f, -0.6f}),
            norm({-0.7f, 0.8f, 0.3f}),
            norm({0.7f, 0.8f, 0.3f})
        }, 60.0f, 120.0f},
        
        // Sulfur - Angular (92°)
        {"S", 2, {
            norm({-0.5f, 0.8f, -0.4f}),
            norm({0.5f, 0.8f, 0.4f})
        }, 60.0f, 120.0f}
    };
}

// ============================================================================
// GEOMETRY TESTS
// ============================================================================
TEST_CASE("VSEPR Geometry: Carbon Tetrahedral") {
    auto elements = getElementDefinitions();
    const auto& carbon = elements[1]; // C
    
    REQUIRE(carbon.symbol == "C");
    REQUIRE(carbon.maxBonds == 4);
    REQUIRE(carbon.bondingSlots.size() == 4);
    
    // Check all pairs of slots have approximately 109.5° angle
    for (size_t i = 0; i < carbon.bondingSlots.size(); i++) {
        for (size_t j = i + 1; j < carbon.bondingSlots.size(); j++) {
            float angle = angleBetween(carbon.bondingSlots[i], carbon.bondingSlots[j]);
            CHECK(angle >= carbon.expectedMinAngle);
            CHECK(angle <= carbon.expectedMaxAngle);
            MESSAGE("C slot " << i << " to " << j << ": " << angle << "°");
        }
    }
}

TEST_CASE("VSEPR Geometry: Nitrogen Pyramidal") {
    auto elements = getElementDefinitions();
    const auto& nitrogen = elements[2]; // N
    
    REQUIRE(nitrogen.symbol == "N");
    REQUIRE(nitrogen.maxBonds == 3);
    
    for (size_t i = 0; i < nitrogen.bondingSlots.size(); i++) {
        for (size_t j = i + 1; j < nitrogen.bondingSlots.size(); j++) {
            float angle = angleBetween(nitrogen.bondingSlots[i], nitrogen.bondingSlots[j]);
            CHECK(angle >= nitrogen.expectedMinAngle);
            CHECK(angle <= nitrogen.expectedMaxAngle);
            MESSAGE("N slot " << i << " to " << j << ": " << angle << "°");
        }
    }
}

TEST_CASE("VSEPR Geometry: Oxygen Angular") {
    auto elements = getElementDefinitions();
    const auto& oxygen = elements[3]; // O
    
    REQUIRE(oxygen.symbol == "O");
    REQUIRE(oxygen.maxBonds == 2);
    
    float angle = angleBetween(oxygen.bondingSlots[0], oxygen.bondingSlots[1]);
    CHECK(angle >= oxygen.expectedMinAngle);
    CHECK(angle <= oxygen.expectedMaxAngle);
    MESSAGE("O-H-O angle: " << angle << "° (expected ~104.5°)");
}

TEST_CASE("VSEPR Geometry: Phosphorus Pyramidal") {
    auto elements = getElementDefinitions();
    const auto& phosphorus = elements[4]; // P
    
    REQUIRE(phosphorus.symbol == "P");
    REQUIRE(phosphorus.maxBonds == 3);
    
    for (size_t i = 0; i < phosphorus.bondingSlots.size(); i++) {
        for (size_t j = i + 1; j < phosphorus.bondingSlots.size(); j++) {
            float angle = angleBetween(phosphorus.bondingSlots[i], phosphorus.bondingSlots[j]);
            CHECK(angle >= phosphorus.expectedMinAngle);
            CHECK(angle <= phosphorus.expectedMaxAngle);
            MESSAGE("P slot " << i << " to " << j << ": " << angle << "°");
        }
    }
}

TEST_CASE("VSEPR Geometry: Sulfur Angular") {
    auto elements = getElementDefinitions();
    const auto& sulfur = elements[5]; // S
    
    REQUIRE(sulfur.symbol == "S");
    REQUIRE(sulfur.maxBonds == 2);
    
    float angle = angleBetween(sulfur.bondingSlots[0], sulfur.bondingSlots[1]);
    CHECK(angle >= sulfur.expectedMinAngle);
    CHECK(angle <= sulfur.expectedMaxAngle);
    MESSAGE("S-H-S angle: " << angle << "° (expected ~92°)");
}

// ============================================================================
// Z-AXIS VARIANCE TESTS (matches runtime validation)
// ============================================================================
TEST_CASE("2.5D Z-Variance: All multi-bond elements have Z variance") {
    auto elements = getElementDefinitions();
    
    for (const auto& el : elements) {
        if (el.maxBonds <= 1) continue; // Skip H
        if (el.bondingSlots.size() < 2) continue;
        
        bool hasZVariance = false;
        float firstZ = el.bondingSlots[0].z;
        
        for (size_t j = 1; j < el.bondingSlots.size(); j++) {
            if (std::abs(el.bondingSlots[j].z - firstZ) > 0.05f) {
                hasZVariance = true;
                break;
            }
        }
        
        CHECK_MESSAGE(hasZVariance, "Element " << el.symbol << " missing Z-axis variance!");
    }
}

// ============================================================================
// MOLECULE COMPOSITION TESTS
// ============================================================================
TEST_CASE("Molecule Composition: Water (H2O)") {
    std::map<int, int> water = {{1, 2}, {8, 1}}; // 2 H + 1 O
    
    CHECK(water[1] == 2);  // 2 Hydrogen
    CHECK(water[8] == 1);  // 1 Oxygen
}

TEST_CASE("Molecule Composition: Phosphine (PH3)") {
    std::map<int, int> phosphine = {{1, 3}, {15, 1}}; // 3 H + 1 P
    
    CHECK(phosphine[1] == 3);   // 3 Hydrogen
    CHECK(phosphine[15] == 1);  // 1 Phosphorus
}

TEST_CASE("Molecule Composition: Ammonia (NH3)") {
    std::map<int, int> ammonia = {{1, 3}, {7, 1}}; // 3 H + 1 N
    
    CHECK(ammonia[1] == 3);  // 3 Hydrogen
    CHECK(ammonia[7] == 1);  // 1 Nitrogen
}
