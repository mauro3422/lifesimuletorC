#include "BondingSystem.hpp"
#include "core/Config.hpp"
#include <cmath>

int BondingSystem::getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& element = db.getElement(atoms[parentId].atomicNumber);
    for (int i = 0; i < (int)element.bondingSlots.size(); i++) {
        bool occupied = false;
        for (const StateComponent& s : states) {
            if (s.isClustered && s.parentEntityId == parentId && s.parentSlotIndex == i) {
                occupied = true;
                break;
            }
        }
        if (!occupied) return i;
    }
    return -1;
}

bool BondingSystem::tryBond(int sourceId, int targetId, 
                           std::vector<StateComponent>& states,
                           const std::vector<AtomComponent>& atoms,
                           const std::vector<TransformComponent>& transforms) {
    if (sourceId < 0 || targetId < 0 || sourceId == targetId) return false;
    if (states[sourceId].isClustered) return false; 

    // SMART SCANNER: Buscamos miembros de la misma molécula
    int molRootId = (states[targetId].moleculeId != -1) ? states[targetId].moleculeId : targetId;
    
    // Lista de candidatos para hostear el enlace (todos los miembros de la molécula)
    std::vector<int> candidates;
    candidates.push_back(molRootId);
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].moleculeId == molRootId) candidates.push_back(i);
    }

    int bestHostId = -1;
    int bestSlotIdx = -1;
    float minSourceDist = 9999.0f;

    for (int hostId : candidates) {
        const TransformComponent& hostTr = transforms[hostId];
        const TransformComponent& sourceTr = transforms[sourceId];
        
        Vector3 relPos = { sourceTr.x - hostTr.x, sourceTr.y - hostTr.y, sourceTr.z - hostTr.z };
        int slotIdx = getBestAvailableSlot(hostId, relPos, states, atoms);
        
        // MODO MAGNETICO: Si el host es el jugador y no tiene hijos, aceptamos cualquier slot libre si esta muy cerca
        if (slotIdx == -1 && hostId == 0) {
            float dist = std::sqrt(relPos.x*relPos.x + relPos.y*relPos.y + relPos.z*relPos.z);
            if (dist < Config::TRACTOR_DOCKING_RANGE * 0.8f) {
                // Buscamos cualquier slot libre sin importar el angulo
                slotIdx = getFirstFreeSlot(hostId, states, atoms);
                if (slotIdx != -1) TraceLog(LOG_INFO, "[BOND] Magnetic docking activado para el Jugador");
            }
        }

        if (slotIdx != -1) {
            float dist = std::sqrt(relPos.x*relPos.x + relPos.y*relPos.y + relPos.z*relPos.z);
            if (dist < minSourceDist) {
                minSourceDist = dist;
                bestHostId = hostId;
                bestSlotIdx = slotIdx;
            }
        }
    }

    if (bestHostId != -1) {
        states[sourceId].isClustered = true;
        states[sourceId].parentEntityId = bestHostId;
        states[sourceId].parentSlotIndex = bestSlotIdx;
        states[sourceId].moleculeId = molRootId;
        states[sourceId].dockingProgress = 0.0f; // Iniciar animacion de acoplamiento
        TraceLog(LOG_INFO, "[BOND] Union exitosa: %d -> %d (Slot %d)", sourceId, bestHostId, bestSlotIdx);
        return true;
    }

    // Si fallamos por distancia o alineación, logueamos discretamente
    if (minSourceDist < 60.0f) {
        TraceLog(LOG_DEBUG, "[BOND] Intento fallido. Distancia: %.2f", minSourceDist);
    }

    return false;
}

int BondingSystem::getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& element = db.getElement(atoms[parentId].atomicNumber);
    
    float len = std::sqrt(relativePos.x*relativePos.x + relativePos.y*relativePos.y + relativePos.z*relativePos.z);
    if (len < 0.001f) return -1;
    Vector3 dir = { relativePos.x/len, relativePos.y/len, relativePos.z/len };

    int bestSlot = -1;
    float maxDot = -2.0f; 

    for (int i = 0; i < (int)element.bondingSlots.size(); i++) {
        bool occupied = false;
        for (const StateComponent& s : states) {
            if (s.isClustered && s.parentEntityId == parentId && s.parentSlotIndex == i) {
                occupied = true;
                break;
            }
        }
        if (occupied) continue;

        Vector3 slotDir = element.bondingSlots[i];
        float dot = dir.x*slotDir.x + dir.y*slotDir.y + dir.z*slotDir.z;
        if (dot > maxDot) {
            maxDot = dot;
            bestSlot = i;
        }
    }

    if (maxDot < Config::BOND_SNAP_THRESHOLD) {
        // Logueamos solo si la alineación era casi buena (>0.3)
        if (maxDot > 0.3f) TraceLog(LOG_DEBUG, "[BOND] SLOT RECHAZADO: Best dot %.2f < threshold %.2f", maxDot, Config::BOND_SNAP_THRESHOLD);
    }

    return (maxDot > Config::BOND_SNAP_THRESHOLD) ? bestSlot : -1; 
}

void BondingSystem::updateHierarchy(std::vector<TransformComponent>& transforms,
                                   std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();

    for (int i = 0; i < (int)transforms.size(); i++) {
        StateComponent& state = states[i];
        if (state.isClustered && state.parentEntityId != -1) {
            int pId = state.parentEntityId;
            const TransformComponent& parentTr = transforms[pId];
            const Element& parentElement = db.getElement(atoms[pId].atomicNumber);

            if (state.parentSlotIndex >= 0 && state.parentSlotIndex < (int)parentElement.bondingSlots.size()) {
                Vector3 slotDir = parentElement.bondingSlots[state.parentSlotIndex];
                
                // Usamos el radio VISUAL (igual que el renderer) para el calculo de distancia
                float parentRadius = parentElement.vdWRadius * Config::BASE_ATOM_RADIUS;
                float childRadius = db.getElement(atoms[i].atomicNumber).vdWRadius * Config::BASE_ATOM_RADIUS;
                float bondDist = (parentRadius + childRadius) * Config::BOND_COMPRESSION;

                // Posicion objetivo (donde deberia estar el atomo anclado)
                float targetX = parentTr.x + slotDir.x * bondDist;
                float targetY = parentTr.y + slotDir.y * bondDist;
                float targetZ = parentTr.z + slotDir.z * bondDist;
                
                // Animacion suave: interpolamos hacia la posicion objetivo
                // dockingProgress va de 0 (recien unido) a 1 (totalmente acoplado)
                const float dockingSpeed = 0.08f; // Ajustar para velocidad de acoplamiento
                if (state.dockingProgress < 1.0f) {
                    state.dockingProgress += dockingSpeed;
                    if (state.dockingProgress > 1.0f) state.dockingProgress = 1.0f;
                }
                
                // Lerp suave hacia la posicion final
                float t = state.dockingProgress;
                transforms[i].x += (targetX - transforms[i].x) * t * 0.2f;
                transforms[i].y += (targetY - transforms[i].y) * t * 0.2f;
                transforms[i].z += (targetZ - transforms[i].z) * t * 0.2f;
                
                // Velocidad suave (hereda la del padre gradualmente)
                transforms[i].vx += (parentTr.vx - transforms[i].vx) * t * 0.3f;
                transforms[i].vy += (parentTr.vy - transforms[i].vy) * t * 0.3f;
                transforms[i].vz += (parentTr.vz - transforms[i].vz) * t * 0.3f;
            }
        }
    }
}

void BondingSystem::updateSpontaneousBonding(std::vector<StateComponent>& states,
                                              const std::vector<AtomComponent>& atoms,
                                              const std::vector<TransformComponent>& transforms) {
    // Por cada átomo libre, buscamos vecinos cercanos para intentar un enlace espontáneo
    for (int i = 1; i < (int)states.size(); i++) { // Empezamos desde 1 para excluir al jugador
        if (states[i].isClustered) continue; // Ya está enlazado, skip

        for (int j = i + 1; j < (int)states.size(); j++) {
            // Solo intentamos unir un átomo libre a otro átomo (libre o molécula)
            // Usamos el átomo con menor ID como potencial "padre" para simplificar
            
            float dx = transforms[i].x - transforms[j].x;
            float dy = transforms[i].y - transforms[j].y;
            float dz = transforms[i].z - transforms[j].z;
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

            if (dist < Config::BOND_AUTO_RANGE) {
                // Intentamos unir i a j (j como target/potencial padre)
                if (!states[j].isClustered) {
                    // Ambos libres: Intentamos crear un dimero. j será el padre.
                    if (tryBond(i, j, states, atoms, transforms)) {
                        // Éxito: el átomo i se unió a j
                        break; // Pasamos al siguiente átomo libre
                    }
                } else {
                    // j ya está en una molécula: Intentamos que i se una a esa molécula
                    if (tryBond(i, j, states, atoms, transforms)) {
                        break;
                    }
                }
            }
        }
    }
}
