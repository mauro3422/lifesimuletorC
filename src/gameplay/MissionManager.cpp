#include "MissionManager.hpp"
#include "../ui/NotificationManager.hpp"
#include "../core/Config.hpp"
#include "DiscoveryLog.hpp"

void MissionManager::initialize() {
    loadMissions();
}

void MissionManager::loadMissions() {
    missions.push_back({
        "m_h2", "Formar Hidrógeno (H2)", 
        "Dos átomos de hidrógeno unidos. La base de la simplicidad.",
        "El hidrógeno es el elemento más abundante del universo. En este caldo primordial, su unión en H2 es el primer paso hacia estructuras más complejas. Representa la estabilidad mínima necesaria para la existencia.",
        "+30 ATP", 0, MissionStatus::ACTIVE
    });

    missions.push_back({
        "m_h2o", "Formar Agua (H2O)", 
        "La molécula de la vida. Estructura angular crítica.",
        "El agua permite la solvatación de químicos orgánicos, facilitando las reacciones que darán lugar a la vida.",
        "+50 ATP", 0, MissionStatus::AVAILABLE
    });

    missions.push_back({
        "m_adenine", "Formar Adenina (C5H5N5)", 
        "Base nitrogenada fundamental para el código genético.",
        "La adenina es una de las cuatro bases del ADN y ARN. Su formación espontánea es un milagro de la química prebiótica.",
        "+200 ATP", 1, MissionStatus::LOCKED
    });
}

void MissionManager::update(float dt) {
    // Lógica para verificar condiciones si no son disparadas por eventos
}

void MissionManager::activateMission(const std::string& id) {
    for (auto& m : missions) {
        if (m.id == id && m.status == MissionStatus::AVAILABLE) {
            m.status = MissionStatus::ACTIVE;
        }
    }
}

void MissionManager::completeMission(const std::string& id) {
    for (auto& m : missions) {
        if (m.id == id && m.status == MissionStatus::ACTIVE) {
            m.status = MissionStatus::COMPLETED;
            NotificationManager::getInstance().show("Misión Completada: " + m.title, LIME);
            // Desbloquear siguientes misiones o dar recompensas
        }
    }
}

void MissionManager::notifyBondCreated(int atomicNumberA, int atomicNumberB) {
    DiscoveryLog::getInstance().discoverElement(atomicNumberA);
    DiscoveryLog::getInstance().discoverElement(atomicNumberB);
    
    if (atomicNumberA == 1 && atomicNumberB == 1) {
        completeMission("m_h2");
    }
}

void MissionManager::notifyMoleculeDiscovered(const std::string& molName) {
    if (molName == "Agua" || molName == "H2O") {
        completeMission("m_h2o");
    }
}
