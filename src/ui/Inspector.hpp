#ifndef INSPECTOR_HPP
#define INSPECTOR_HPP

#include "chemistry/Element.hpp"
#include "chemistry/Molecule.hpp"
#include "input/InputHandler.hpp"
#include "ecs/components.hpp"
#include "raylib.h"
#include <map>

class Inspector {
public:
    Inspector();
    void draw(const Element& element, int entityID, InputHandler& input, std::vector<StateComponent>& states, std::vector<AtomComponent>& atoms);
    
    void setMolecule(const Molecule* mol) { currentMolecule = mol; }
    void setComposition(const std::map<int, int>& comp) { currentComposition = comp; }

private:
    void drawElementCard(const Element& element, float x, float y, float size, InputHandler& input);
    void drawMoleculeOverlay(Rectangle rect, InputHandler& input);
    void drawTransitoryMoleculeOverlay(Rectangle rect, InputHandler& input);
    
    const Molecule* currentMolecule = nullptr;
    std::map<int, int> currentComposition;
};

#endif
