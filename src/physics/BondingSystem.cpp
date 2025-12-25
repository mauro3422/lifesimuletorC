#include "BondingSystem.hpp"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"
#include <cmath>

int BondingSystem::getFirstFreeSlot(int parentId, const std::vector<StateComponent>& states, const std::vector<AtomComponent>& atoms) {
    if (parentId < 0 || parentId >= (int)states.size()) return -1;

    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& element = db.getElement(atoms[parentId].atomicNumber);
    
    // VALIDACIÓN DE VALENCIA TOTAL (Entrantes + Salientes)
    int currentBonds = 0;
    if (states[parentId].parentEntityId != -1) currentBonds++; // Enlace con el padre

    // Contar hijos existentes
    for (const StateComponent& s : states) {
        if (s.isClustered && s.parentEntityId == parentId) {
            currentBonds++;
        }
    }

    // Si ya alcanzó su límite químico (maxBonds), no ofrece más slots
    if (currentBonds >= element.maxBonds) return -1;

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

BondingSystem::BondError BondingSystem::tryBond(int sourceId, int targetId, 
                           std::vector<StateComponent>& states,
                           const std::vector<AtomComponent>& atoms,
                           const std::vector<TransformComponent>& transforms,
                           bool forced) {
    if (sourceId < 0 || targetId < 0 || sourceId == targetId) return INTERNAL_ERROR;
    if (states[sourceId].isClustered) return ALREADY_CLUSTERED; 

    // SMART SCANNER: Buscamos TODOS los miembros de la misma molécula usando jerarquía dinámica
    int molRootId = MathUtils::findMoleculeRoot(targetId, states);
    
    std::vector<int> candidates;
    for (int i = 0; i < (int)states.size(); i++) {
        // Todo átomo que comparta el mismo root pertenece a esta molécula
        if (MathUtils::findMoleculeRoot(i, states) == molRootId) {
            candidates.push_back(i);
        }
    }

    int bestHostId = -1;
    int bestSlotIdx = -1;
    float minSourceDist = 9999.0f;
    bool moleculeHasAnyFreeSlot = false;

    for (int hostId : candidates) {
        const TransformComponent& hostTr = transforms[hostId];
        const TransformComponent& sourceTr = transforms[sourceId];
        
        Vector3 relPos = { sourceTr.x - hostTr.x, sourceTr.y - hostTr.y, sourceTr.z - hostTr.z };
        
        // Verificamos si este host tiene ALGUN slot libre antes de evaluar el angulo
        if (getFirstFreeSlot(hostId, states, atoms) != -1) {
            moleculeHasAnyFreeSlot = true;
        }

        // El modo forced ignora el ángulo en la búsqueda del mejor slot
        int slotIdx = getBestAvailableSlot(hostId, relPos, states, atoms, forced);
        
        // AUTO-ACOMODAMIENTO: Si el modo es forzado y no encontramos slot por ángulo,
        // intentamos buscar simplemente el primer slot libre en este host de la molécula.
        if (slotIdx == -1 && forced) {
            slotIdx = getFirstFreeSlot(hostId, states, atoms);
        }

        if (slotIdx != -1) {
            float dist = std::sqrt(relPos.x*relPos.x + relPos.y*relPos.y + relPos.z*relPos.z);
            // Prioridad: El mas cercano espacialmente
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
        states[sourceId].dockingProgress = 0.0f; 
        
        // --- CÁLCULO DE POLARIDAD (Carga Parcial) ---
        float enHost = ChemistryDatabase::getInstance().getElement(atoms[bestHostId].atomicNumber).electronegativity;
        float enSource = ChemistryDatabase::getInstance().getElement(atoms[sourceId].atomicNumber).electronegativity;
        float polarity = (enHost - enSource) * 0.15f; 
        const_cast<AtomComponent&>(atoms[bestHostId]).partialCharge += polarity;
        const_cast<AtomComponent&>(atoms[sourceId]).partialCharge -= polarity;

        TraceLog(LOG_INFO, "[BOND] GLOBAL SUCCESS: %d -> %d (Molecule %d) Polarity: %.2f", sourceId, bestHostId, molRootId, polarity);
        return SUCCESS;
    }

    // --- LÓGICA DE INSERCIÓN UNIVERSAL (Splice Bonding) ---
    // Si la molecula está saturada pero el jugador fuerza la unión (Tractor Beam),
    // aplicamos reglas de "Intercepción de Enlaces" basadas en valencia.
    if (forced) {
        const Element& sourceElem = ChemistryDatabase::getInstance().getElement(atoms[sourceId].atomicNumber);
        
        // Regla Universal A: Solo átomos capaces de formar puentes (valencia >= 2) pueden "empalmar".
        if (sourceElem.maxBonds >= 2) {
            int closestHost = -1;
            float minDist = 9999.0f;
            for (int hostId : candidates) {
                float dx = transforms[sourceId].x - transforms[hostId].x;
                float dy = transforms[sourceId].y - transforms[hostId].y;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist < minDist) {
                    minDist = dist;
                    closestHost = hostId;
                }
            }

            if (closestHost != -1) {
                // Algoritmo de Intercepción:
                // Buscamos cualquier conexión existente del host para insertar el nuevo átomo en medio.
                
                int connectionId = -1;
                bool isParentConn = false;

                if (states[closestHost].parentEntityId != -1) {
                    connectionId = states[closestHost].parentEntityId;
                    isParentConn = true;
                } else {
                    // Si el host es root, buscamos su primer hijo
                    for (int i = 0; i < (int)states.size(); i++) {
                        if (states[i].isClustered && states[i].parentEntityId == closestHost) {
                            connectionId = i;
                            isParentConn = false;
                            break;
                        }
                    }
                }

                if (connectionId != -1) {
                    // REGLA DE INTEGRIDAD: El átomo que queda como puente DEBE tener valencia >= 2.
                    // No podemos "empujar" un hidrógeno para que sea puente.
                    const Element& connElem = ChemistryDatabase::getInstance().getElement(atoms[connectionId].atomicNumber);
                    const Element& hostElem = ChemistryDatabase::getInstance().getElement(atoms[closestHost].atomicNumber);
                    
                    if (isParentConn) {
                        // CASO A: Insertar entre Host y su Padre (Parent -> Source -> Host)
                        // Verificamos si el Source (nuevo puente) tiene valencia para aguantar a ambos.
                        if (sourceElem.maxBonds < 2) return VALENCY_FULL; // El hidrógeno no puede ser puente

                        int oldParentId = states[closestHost].parentEntityId;
                        int oldSlot = states[closestHost].parentSlotIndex;

                        states[closestHost].parentEntityId = sourceId;
                        states[closestHost].parentSlotIndex = 0; // Se inserta en el brazo inicial del puente

                        states[sourceId].isClustered = true;
                        states[sourceId].parentEntityId = oldParentId;
                        states[sourceId].parentSlotIndex = oldSlot;
                        states[sourceId].moleculeId = molRootId;
                        states[sourceId].dockingProgress = 0.0f;
                    } else {
                        // CASO B: Insertar entre Host y su Hijo (Host -> Source -> Hijo)
                        int oldChildId = connectionId;
                        int oldSlot = states[oldChildId].parentSlotIndex;

                        states[oldChildId].parentEntityId = sourceId;
                        states[oldChildId].parentSlotIndex = 0;

                        states[sourceId].isClustered = true;
                        states[sourceId].parentEntityId = closestHost;
                        states[sourceId].parentSlotIndex = oldSlot;
                        states[sourceId].moleculeId = molRootId;
                        states[sourceId].dockingProgress = 0.0f;
                    }
                    
                    TraceLog(LOG_INFO, "[BOND] UNIVERSAL SPLICE: Atomo %d insertado como puente en la molecula %d", sourceId, molRootId);
                    
                    // --- CÁLCULO DE POLARIDAD (Carga Parcial) ---
                    float enHost = ChemistryDatabase::getInstance().getElement(atoms[closestHost].atomicNumber).electronegativity;
                    float enSource = ChemistryDatabase::getInstance().getElement(atoms[sourceId].atomicNumber).electronegativity;
                    float polarity = (enHost - enSource) * 0.15f; // Coeficiente de polaridad
                    const_cast<AtomComponent&>(atoms[closestHost]).partialCharge += polarity;
                    const_cast<AtomComponent&>(atoms[sourceId]).partialCharge -= polarity;

                    return SUCCESS;
                }
            }
        }
    }

    return moleculeHasAnyFreeSlot ? ANGLE_INCOMPATIBLE : VALENCY_FULL;
}

int BondingSystem::getBestAvailableSlot(int parentId, Vector3 relativePos,
                                   const std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms,
                                   bool ignoreAngle) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    const Element& element = db.getElement(atoms[parentId].atomicNumber);
    
    float len = std::sqrt(relativePos.x*relativePos.x + relativePos.y*relativePos.y + relativePos.z*relativePos.z);
    if (len < 0.001f) return -1;
    Vector3 dir = { relativePos.x/len, relativePos.y/len, relativePos.z/len };

    // VALIDACIÓN DE VALENCIA TOTAL
    int currentBonds = 0;
    if (states[parentId].parentEntityId != -1) currentBonds++;
    for (const StateComponent& s : states) {
        if (s.isClustered && s.parentEntityId == parentId) currentBonds++;
    }

    if (currentBonds >= element.maxBonds) return -1;

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

    if (bestSlot == -1) return -1; // No hay slots de valencia
    
    // Si ignoramos el ángulo (Modo Forzado), devolvemos el mejor slot disponible
    if (ignoreAngle) return bestSlot;

    // Si no, verificamos el umbral geométrico (Modo Natural/NPC)
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
                
                float parentRadius = parentElement.vdWRadius * Config::BASE_ATOM_RADIUS;
                float childRadius = db.getElement(atoms[i].atomicNumber).vdWRadius * Config::BASE_ATOM_RADIUS;
                float bondDist = (parentRadius + childRadius) * Config::BOND_COMPRESSION;

                // --- ACOPLAMIENTO ELÁSTICO (Manejado ahora por PhysicsEngine) ---
                // Ya no forzamos la posición directamente aquí para permitir vibración y estrés.
                // Simplemente actualizamos el dockingProgress para animaciones visuales.
                
                if (state.dockingProgress < 1.0f) {
                    state.dockingProgress += Config::BOND_DOCKING_SPEED;
                    if (state.dockingProgress > 1.0f) state.dockingProgress = 1.0f;
                }
                
                // Las fuerzas matemáticas de restauración se aplican en PhysicsEngine::step
            }
        }
    }
}

void BondingSystem::updateSpontaneousBonding(std::vector<StateComponent>& states,
                                               const std::vector<AtomComponent>& atoms,
                                               const std::vector<TransformComponent>& transforms,
                                               int tractedEntityId) {
    
    int tractedRoot = (tractedEntityId != -1) ? MathUtils::findMoleculeRoot(tractedEntityId, states) : -1;

    for (int i = 1; i < (int)states.size(); i++) { 
        if (states[i].isClustered) continue; 

        // Si el átomo i está siendo arrastrado (o es parte de una molécula arrastrada), lo ignoramos
        if (tractedRoot != -1 && MathUtils::findMoleculeRoot(i, states) == tractedRoot) continue;

        for (int j = i + 1; j < (int)states.size(); j++) {
            float dx = transforms[i].x - transforms[j].x;
            float dy = transforms[i].y - transforms[j].y;
            float dz = transforms[i].z - transforms[j].z;
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

            if (dist < Config::BOND_AUTO_RANGE) {
                // EXCLUSIÓN: 
                // 1. Ignoramos si alguno pertenece a la molécula del jugador (ID 0).
                // 2. Ignoramos si alguno pertenece a la molécula que estamos arrastrando con el tractor.
                int rootI = MathUtils::findMoleculeRoot(i, states);
                int rootJ = MathUtils::findMoleculeRoot(j, states);

                if (rootI == 0 || rootJ == 0) continue;
                if (tractedRoot != -1 && (rootI == tractedRoot || rootJ == tractedRoot)) continue;

                if (tryBond(i, j, states, atoms, transforms, false) == SUCCESS) {
                    break;
                }
            }
        }
    }
}
