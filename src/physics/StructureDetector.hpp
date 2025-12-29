#ifndef STRUCTURE_DETECTOR_HPP
#define STRUCTURE_DETECTOR_HPP

#include <vector>
#include <algorithm>
#include <cmath>
#include <map>
#include "../ecs/components.hpp"
#include "../chemistry/StructureRegistry.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "RingChemistry.hpp"

/**
 * StructureDetector (Phase 41)
 * Detects when organic bonds can form a known structure,
 * then reorganizes hierarchy and closes the ring.
 */
class StructureDetector {
public:

    /**
     * Try to form any valid structure from the molecule rooted at rootId.
     * Returns true if a structure was formed.
     */
    static bool tryFormStructure(int rootId,
                                  std::vector<StateComponent>& states,
                                  std::vector<AtomComponent>& atoms,
                                  std::vector<TransformComponent>& transforms) {
        
        // 1. Get all atoms in this molecule
        std::vector<int> members = getMoleculeMembers(rootId, states);
        if (members.size() < 4) return false;  // Minimum for any ring
        
        // 2. Group by atomic number
        std::map<int, std::vector<int>> byElement;
        for (int id : members) {
            byElement[atoms[id].atomicNumber].push_back(id);
        }
        
        // 3. Check each structure definition
        const auto& registry = StructureRegistry::getInstance();
        for (const auto& def : registry.getAllStructures()) {
            auto it = byElement.find(def.atomicNumber);
            if (it == byElement.end()) continue;
            
            int numCandidates = (int)it->second.size();
            // Removed per-frame debug log - was causing massive lag
            // Log only when structure actually forms (in reorganizeAndClose)
            
            if (numCandidates >= def.atomCount) {
                // We have enough atoms of this type!
                std::vector<int> candidates = it->second;
                
                // 4. Check if they're all terminal (can form ring)
                if (canFormRing(candidates, states, def.atomCount)) {
                    // 5. Reorganize and close
                    if (reorganizeAndClose(candidates, states, atoms, transforms, def)) {
                        return true;
                    }
                } else {
                    TraceLog(LOG_INFO, "[STRUCTURE] Cannot form ring: some atoms already in ring");
                }
            }
        }
        
        return false;
    }

private:
    /**
     * Get all atoms in a molecule by traversing hierarchy
     */
    static std::vector<int> getMoleculeMembers(int rootId, const std::vector<StateComponent>& states) {
        std::vector<int> members;
        std::vector<bool> visited(states.size(), false);
        std::vector<int> stack = {rootId};
        
        while (!stack.empty()) {
            int curr = stack.back();
            stack.pop_back();
            
            if (curr < 0 || curr >= (int)states.size() || visited[curr]) continue;
            visited[curr] = true;
            members.push_back(curr);
            
            // Add parent
            if (states[curr].parentEntityId != -1 && !visited[states[curr].parentEntityId]) {
                stack.push_back(states[curr].parentEntityId);
            }
            
            // Add children (by scanning)
            for (int i = 0; i < (int)states.size(); i++) {
                if (states[i].parentEntityId == curr && !visited[i]) {
                    stack.push_back(i);
                }
            }
        }
        
        return members;
    }
    
    /**
     * Check if N atoms can form a ring (no existing cycle bonds, enough are connected)
     */
    static bool canFormRing(const std::vector<int>& candidates, 
                            const std::vector<StateComponent>& states,
                            int requiredCount) {
        if ((int)candidates.size() < requiredCount) return false;
        
        // Check none already in a ring
        for (int id : candidates) {
            if (states[id].isInRing || states[id].cycleBondId != -1) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Reorganize hierarchy to form linear chain and close as ring
     */
    static bool reorganizeAndClose(std::vector<int> candidates,
                                   std::vector<StateComponent>& states,
                                   std::vector<AtomComponent>& atoms,
                                   std::vector<TransformComponent>& transforms,
                                   const StructureDefinition& def) {
        
        int n = def.atomCount;
        if ((int)candidates.size() < n) return false;
        
        // Take first N candidates
        candidates.resize(n);
        
        // 1. Calculate centroid
        float cx = 0, cy = 0;
        for (int id : candidates) {
            cx += transforms[id].x;
            cy += transforms[id].y;
        }
        cx /= n;
        cy /= n;
        
        // 2. Sort by angle from centroid (creates natural ring order)
        std::sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            float angleA = std::atan2(transforms[a].y - cy, transforms[a].x - cx);
            float angleB = std::atan2(transforms[b].y - cy, transforms[b].x - cx);
            return angleA < angleB;
        });
        
        // 3. Reorganize hierarchy as linear chain: 0→1→2→3→4→5
        // First, break all existing bonds between these atoms
        for (int id : candidates) {
            if (states[id].parentEntityId != -1) {
                // Check if parent is in our set
                bool parentInSet = std::find(candidates.begin(), candidates.end(), 
                                              states[id].parentEntityId) != candidates.end();
                if (parentInSet) {
                    int parentId = states[id].parentEntityId;
                    states[parentId].childCount--;
                    states[id].parentEntityId = -1;
                    states[id].isClustered = false;
                }
            }
        }
        
        // 4. Create new linear chain
        for (int i = 1; i < n; i++) {
            int child = candidates[i];
            int parent = candidates[i - 1];
            
            states[child].parentEntityId = parent;
            states[child].isClustered = true;
            states[parent].childCount++;
        }
        
        // 5. Close cycle between first and last
        int first = candidates[0];
        int last = candidates[n - 1];
        
        // Call tryCycleBond to handle ring formation
        if (RingChemistry::tryCycleBond(first, last, states, atoms, transforms) == BondError::SUCCESS) {
            TraceLog(LOG_INFO, "[STRUCTURE] Formed %s from %d atoms via detection", 
                     def.name.c_str(), n);
            return true;
        }
        
        return false;
    }
};

#endif // STRUCTURE_DETECTOR_HPP
