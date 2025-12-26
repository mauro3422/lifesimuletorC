#ifndef MISSION_MANAGER_HPP
#define MISSION_MANAGER_HPP

#include <string>
#include <vector>
#include <map>

enum class MissionStatus {
    LOCKED,
    AVAILABLE,
    ACTIVE,
    COMPLETED
};

struct Mission {
    std::string id;
    std::string title;
    std::string description;
    std::string scientificContext;
    std::string reward;
    int tier;
    MissionStatus status;
};

class MissionManager {
public:
    static MissionManager& getInstance() {
        static MissionManager instance;
        return instance;
    }

    void initialize();
    void update(float dt);
    
    const std::vector<Mission>& getMissions() const { return missions; }
    void activateMission(const std::string& id);
    void completeMission(const std::string& id);
    
    // Checkers para disparar misiones según la química
    void notifyBondCreated(int atomicNumberA, int atomicNumberB);
    void notifyMoleculeDiscovered(const std::string& molName);

private:
    MissionManager() {}
    std::vector<Mission> missions;
    
    void loadMissions();
};

#endif
