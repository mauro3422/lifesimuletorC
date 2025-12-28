#include "LocalizationManager.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <mutex>

using json = nlohmann::json;

// ... (Top of file stays same, using std::lock_guard)

void LocalizationManager::setLanguage(const std::string& langCode) {
    std::lock_guard<std::mutex> lock(trMutex);
    currentLanguage = langCode;
    std::string path = "data/lang_" + langCode + ".json";
    
    if (!loadLanguageFile(path)) {
        TraceLog(LOG_WARNING, "[LOCALIZATION] Could not load %s, falling back to English", path.c_str());
        if (langCode != "en") {
            loadLanguageFile("data/lang_en.json");
        }
    }
}

// loadLanguageFile is private and called inside lock, so no lock needed here, 
// BUT we must be careful not to call public methods that lock from here (none called).
bool LocalizationManager::loadLanguageFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    json data;
    try {
        file >> data;
        strings.clear();
        for (auto it = data.begin(); it != data.end(); ++it) {
            strings[it.key()] = it.value().get<std::string>();
        }
        TraceLog(LOG_INFO, "[LOCALIZATION] Loaded %d strings from %s", (int)strings.size(), path.c_str());
        return true;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[LOCALIZATION] Error parsing %s: %s", path.c_str(), e.what());
        return false;
    }
}

std::string LocalizationManager::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(trMutex); 
    auto it = strings.find(key);
    if (it != strings.end()) {
        return it->second;
    }
    return key; // Return key as fallback
}
