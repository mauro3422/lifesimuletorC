#include "MissionManager.hpp"
#include "../ui/NotificationManager.hpp"
#include "../core/Config.hpp"
#include "DiscoveryLog.hpp"
#include "../core/LocalizationManager.hpp"
#include "../core/JsonLoader.hpp"

void MissionManager::initialize() {
    reload();
}

void MissionManager::reload() {
    missions.clear();
    loadMissions();
}

void MissionManager::loadMissions() {
    try {
        std::string lang = LocalizationManager::getInstance().getLanguageCode();
        missions = JsonLoader::loadMissions("data/missions.json", lang);
        TraceLog(LOG_INFO, "[MISSIONS] Loaded %d missions from JSON (Language: %s)", (int)missions.size(), lang.c_str());
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[MISSIONS] Failed to load missions.json: %s", e.what());
    }
}

void MissionManager::update(float dt) {
    // Logic to verify conditions if not triggered by events
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
            std::string msg = LocalizationManager::getInstance().get("ui.notification.mission_completed");
            NotificationManager::getInstance().show(msg + " " + m.title, LIME);
            // Unlock next missions or grant rewards
        }
    }
}

void MissionManager::notifyBondCreated(int atomicNumberA, int atomicNumberB) {
    DiscoveryLog::getInstance().discoverElement(atomicNumberA);
    DiscoveryLog::getInstance().discoverElement(atomicNumberB);
    
    // Check for specific primordial bonds (IDs are better than matching numbers)
    if ((atomicNumberA == 1 && atomicNumberB == 1)) {
        completeMission("m_h2");
    }
}

void MissionManager::notifyMoleculeDiscovered(const std::string& moleculeId) {
    if (moleculeId == "water") {
        completeMission("m_h2o");
    } else if (moleculeId == "methane") {
        completeMission("m_ch4");
    }
    // Add more discovery triggers as needed
}
