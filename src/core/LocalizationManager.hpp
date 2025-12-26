#ifndef LOCALIZATION_MANAGER_HPP
#define LOCALIZATION_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include "raylib.h"

class LocalizationManager {
public:
    static LocalizationManager& getInstance() {
        static LocalizationManager instance;
        return instance;
    }

    // Set the active language (e.g., "es", "en")
    void setLanguage(const std::string& langCode);
    
    // Get a translated string by key
    std::string get(const std::string& key) const;

    // Direct access to current language code
    std::string getLanguageCode() const { return currentLanguage; }

private:
    LocalizationManager() : currentLanguage("es") {}
    
    std::string currentLanguage;
    std::unordered_map<std::string, std::string> strings;

    bool loadLanguageFile(const std::string& path);
};

#endif
