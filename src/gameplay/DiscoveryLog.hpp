#ifndef DISCOVERY_LOG_HPP
#define DISCOVERY_LOG_HPP

#include <set>
#include <string>
#include <vector>

class DiscoveryLog {
public:
    static DiscoveryLog& getInstance() {
        static DiscoveryLog instance;
        return instance;
    }

    void discoverElement(int atomicNumber) {
        discoveredElements.insert(atomicNumber);
    }

    void discoverMolecule(const std::string& id) {
        discoveredMolecules.insert(id);
    }

    bool isElementDiscovered(int atomicNumber) const {
        return discoveredElements.count(atomicNumber) > 0;
    }

    bool isMoleculeDiscovered(const std::string& id) const {
        return discoveredMolecules.count(id) > 0;
    }

    const std::set<int>& getDiscoveredElements() const { return discoveredElements; }
    const std::set<std::string>& getDiscoveredMolecules() const { return discoveredMolecules; }

private:
    DiscoveryLog() {}
    std::set<int> discoveredElements;
    std::set<std::string> discoveredMolecules;
};

#endif
