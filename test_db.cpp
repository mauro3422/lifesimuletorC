#include "chemistry/ChemistryDatabase.hpp"
#include <iostream>

int main() {
    std::cout << "Testing Singleton..." << std::endl;
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    std::cout << "Success!" << std::endl;
    return 0;
}
