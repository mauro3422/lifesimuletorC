#include "../physics/BondingSystem.hpp"
#include "../physics/BondingCore.hpp"
#include <iostream>

int main() {
    std::cout << "BondingCore compile check" << std::endl;
    std::vector<StateComponent> states;
    std::vector<AtomComponent> atoms;
    BondingCore::breakBond(0, states, atoms);
    return 0;
}
