#include <iostream>
#include <vector>
#include <cassert>
#include "src/ecs/components.hpp"
#include "src/physics/BondingSystem.hpp"
#include "src/chemistry/ChemistryDatabase.hpp"
#include "src/core/Config.hpp"

// Mock de Raylib GetRandomValue si es necesario
// (En este caso usaremos BondingSystem que es determinista)

int main() {
    std::cout << "🧪 [TEST] Iniciando verificacion de BondingSystem..." << std::endl;

    std::vector<TransformComponent> transforms;
    std::vector<AtomComponent> atoms;
    std::vector<StateComponent> states;

    // 1. Setup: Átomo A (Carbono) en (0,0)
    transforms.push_back({ 0, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 6, 0.0f }); // Carbono
    states.push_back({ false, -1, -1, -1 });

    // 2. Setup: Átomo B (Hidrógeno) muy cerca en la dirección del slot 0
    // Slots de C: [1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0] (simplificado)
    transforms.push_back({ 10.0f, 0, 0, 0, 0, 0, 0 });
    atoms.push_back({ 1, 0.0f }); // Hidrógeno
    states.push_back({ false, -1, -1, -1 });

    std::cout << "[TEST] Intentando unir H (ID 1) a C (ID 0)..." << std::endl;

    // Ejecutamos logic
    bool success = BondingSystem::tryBond(1, 0, states, atoms, transforms);

    if (success) {
        std::cout << "✅ [SUCCESS] BondingSystem reporta EXITO." << std::endl;
        assert(states[1].isClustered == true);
        assert(states[1].parentEntityId == 0);
        std::cout << "[TEST] Slot asignado: " << states[1].parentSlotIndex << std::endl;
        
        // Test de Jerarquía
        BondingSystem::updateHierarchy(transforms, states, atoms);
        std::cout << "[TEST] Nueva posicion del hijo: (" << transforms[1].x << ", " << transforms[1].y << ")" << std::endl;
    } else {
        std::cout << "❌ [FAILURE] BondingSystem rechazo la union." << std::endl;
    }

    return 0;
}
