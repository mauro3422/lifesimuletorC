#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/ecs/components.hpp"
#include "../src/physics/BondingSystem.hpp"
#include "../src/chemistry/ChemistryDatabase.hpp"
#include "../src/core/Config.hpp"

// =============================================================================
// BONDING SYSTEM TESTS
// =============================================================================

TEST_CASE("BondingSystem: Basic H-C bond formation") {
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;

    // Setup: Carbon at (0,0)
    transforms.push_back({ 0, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 6, 0.0f }); // Carbon
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // Setup: Hydrogen near Carbon (within bond range)
    transforms.push_back({ 10.0f, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f }); // Hydrogen
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // Test bond formation
    auto result = BondingSystem::tryBond(1, 0, states, atoms, transforms);
    
    CHECK(result == BondingSystem::SUCCESS);
    CHECK(states[1].isClustered == true);
    CHECK(states[1].parentEntityId == 0);
    CHECK(states[1].parentSlotIndex >= 0);
}

TEST_CASE("BondingSystem: Already clustered atom cannot bond again") {
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;

    // Carbon
    transforms.push_back({ 0, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 6, 0.0f });
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // Hydrogen already bonded
    transforms.push_back({ 10.0f, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f });
    states.push_back({ true, 0, 0, 0, 1.0f, false }); // Already clustered!

    auto result = BondingSystem::tryBond(1, 0, states, atoms, transforms);
    
    CHECK(result == BondingSystem::ALREADY_CLUSTERED);
}

TEST_CASE("BondingSystem: Hydrogen max bonds = 1") {
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;

    // H1 at origin
    transforms.push_back({ 0, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f }); // Hydrogen can only bond once
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // H2 nearby
    transforms.push_back({ 10.0f, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f });
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // H3 also nearby
    transforms.push_back({ -10.0f, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f });
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // First bond should succeed
    auto result1 = BondingSystem::tryBond(1, 0, states, atoms, transforms);
    CHECK(result1 == BondingSystem::SUCCESS);

    // Second bond to same H should fail (valency full)
    auto result2 = BondingSystem::tryBond(2, 0, states, atoms, transforms);
    CHECK(result2 == BondingSystem::VALENCY_FULL);
}

TEST_CASE("BondingSystem: breakBond correctly liberates atom") {
    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;

    // Carbon
    transforms.push_back({ 0, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 6, 0.0f });
    states.push_back({ false, -1, -1, -1, 0.0f, false });

    // Hydrogen bonded to Carbon
    transforms.push_back({ 10.0f, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f });
    states.push_back({ true, 0, 0, 0, 1.0f, false });

    BondingSystem::breakBond(1, states, atoms);

    CHECK(states[1].isClustered == false);
    CHECK(states[1].parentEntityId == -1);
}

// =============================================================================
// CONFIG TESTS
// =============================================================================

TEST_CASE("Config: Throttle frames constant is reasonable") {
    CHECK(Config::BONDING_THROTTLE_FRAMES > 0);
    CHECK(Config::BONDING_THROTTLE_FRAMES <= 60); // At most once per second
}

TEST_CASE("Config: Physics constants are positive") {
    CHECK(Config::COULOMB_CONSTANT > 0);
    CHECK(Config::BOND_SPRING_K > 0);
    CHECK(Config::BOND_AUTO_RANGE > 0);
}
