#ifndef QUIMIDEX_HPP
#define QUIMIDEX_HPP

#include "raylib.h"
#include "../chemistry/ChemistryDatabase.hpp"
#include "../chemistry/Molecule.hpp"
#include "../gameplay/MissionManager.hpp"
#include "../gameplay/DiscoveryLog.hpp"
#include "../input/InputHandler.hpp"
#include <string>
#include <vector>

class Quimidex {
public:
    Quimidex();
    void draw(InputHandler& input);
    void toggle() { isOpen = !isOpen; }
    bool isVisible() const { return isOpen; }

private:
    bool isOpen = false;
    int activeTab = 0; // 0: Molecules, 1: Atoms, 2: Progression
    int selectedElementIdx = 0;
    int selectedMoleculeIdx = 0;
    int selectedMissionIdx = 0;

    std::vector<std::string> tabLabels = { "[M] MOLECULAS", "[A] ATOMOS", "[P] PROGRESION" };
    
    void drawAtomsTab(Rectangle rect, InputHandler& input);
    void drawMoleculesTab(Rectangle rect, InputHandler& input);
    void drawProgressionTab(Rectangle rect, InputHandler& input);
    
    void drawAtomDetail(Rectangle rect, const Element& element, InputHandler& input);
    void drawMoleculeDetail(Rectangle rect, const Molecule& molecule, InputHandler& input);
    void drawMissionDetail(Rectangle rect, const Mission& mission);
};

#endif
