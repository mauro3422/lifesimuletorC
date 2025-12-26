#include "BondingSystem.hpp"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"
#include "gameplay/MissionManager.hpp"
#include "world/EnvironmentManager.hpp"
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
                           std::vector<AtomComponent>& atoms,
                           const std::vector<TransformComponent>& transforms,
                           bool forced,
                           float angleMultiplier) {
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
    float minSourceDist = Config::FLOAT_MAX;
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
        int slotIdx = getBestAvailableSlot(hostId, relPos, states, atoms, forced, angleMultiplier);
        
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
        float polarity = (enHost - enSource) * Config::POLARITY_FACTOR; 
        atoms[bestHostId].partialCharge += polarity;
        atoms[sourceId].partialCharge -= polarity;

        TraceLog(LOG_INFO, "[BOND] GLOBAL SUCCESS: %d -> %d (Molecule %d) Polarity: %.2f", sourceId, bestHostId, molRootId, polarity);
        
        // Notificar al sistema de misiones
        MissionManager::getInstance().notifyBondCreated(atoms[sourceId].atomicNumber, atoms[bestHostId].atomicNumber);
        
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
            float minDist = Config::FLOAT_MAX;
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
                    float polarity = (enHost - enSource) * Config::POLARITY_FACTOR; 
                    atoms[closestHost].partialCharge += polarity;
                    atoms[sourceId].partialCharge -= polarity;

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
                                   bool ignoreAngle,
                                   float angleMultiplier) {
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
    float maxDot = -Config::FLOAT_MAX; 

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
    return (maxDot > (Config::BOND_SNAP_THRESHOLD / angleMultiplier)) ? bestSlot : -1; 
}

void BondingSystem::updateHierarchy(std::vector<TransformComponent>& transforms,
                                   std::vector<StateComponent>& states,
                                   const std::vector<AtomComponent>& atoms) {
    ChemistryDatabase& db = ChemistryDatabase::getInstance();

    for (int i = 0; i < (int)transforms.size(); i++) {
        StateComponent& state = states[i];
        if (state.isClustered && state.parentEntityId != -1) {
            int pId = state.parentEntityId;
            const Element& parentElement = db.getElement(atoms[pId].atomicNumber);

            if (state.parentSlotIndex >= 0 && state.parentSlotIndex < (int)parentElement.bondingSlots.size()) {
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
                                               std::vector<AtomComponent>& atoms,
                                               const std::vector<TransformComponent>& transforms,
                                               EnvironmentManager* env,
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

            float rangeMultiplier = 1.0f;
            float angleMultiplier = 1.0f;
            if (env) {
                rangeMultiplier = env->getBondRangeMultiplier({transforms[i].x, transforms[i].y});
                angleMultiplier = env->getBondAngleMultiplier({transforms[i].x, transforms[i].y});
            }

            if (dist < Config::BOND_AUTO_RANGE * rangeMultiplier) {
                // EXCLUSIÓN: 
                // 1. Ignoramos si alguno pertenece a la molécula del jugador (ID 0).
                // 2. Ignoramos si alguno pertenece a la molécula que estamos arrastrando con el tractor.
                int rootI = MathUtils::findMoleculeRoot(i, states);
                int rootJ = MathUtils::findMoleculeRoot(j, states);

                if (rootI == 0 || rootJ == 0) continue;
                if (tractedRoot != -1 && (rootI == tractedRoot || rootJ == tractedRoot)) continue;
                
                // ESCUDO DE VALENCIA GLOBAL: Si la raíz de la molécula está protegida, nadie se une
                if (states[rootI].isShielded || states[rootJ].isShielded) continue;

                if (tryBond(i, j, states, atoms, transforms, false, angleMultiplier) == SUCCESS) {
                    break;
                }
            }
        }
    }
}
void BondingSystem::breakBond(int entityId, std::vector<StateComponent>& states, 
                              std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;
    StateComponent& state = states[entityId];

    if (!state.isClustered || state.parentEntityId == -1) return;

    int parentId = state.parentEntityId;

    // --- REVERTIR POLARIDAD ---
    // Recuperamos los elementos para recalcular la polaridad que se aplicó originalmente
    float enHost = ChemistryDatabase::getInstance().getElement(atoms[parentId].atomicNumber).electronegativity;
    float enSource = ChemistryDatabase::getInstance().getElement(atoms[entityId].atomicNumber).electronegativity;
    float polarity = (enHost - enSource) * Config::POLARITY_FACTOR; 

    atoms[parentId].partialCharge -= polarity;
    atoms[entityId].partialCharge += polarity;

    // --- LIBERAR ENLACE ---
    state.isClustered = false;
    state.parentEntityId = -1;
    state.parentSlotIndex = -1;
    state.moleculeId = -1; 
    state.dockingProgress = 0.0f;

    TraceLog(LOG_INFO, "[BOND] Enlace roto para atomo %d (liberado de %d)", entityId, parentId);
}

int BondingSystem::findLastChild(int parentId, const std::vector<StateComponent>& states) {
    int lastChild = -1;
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isClustered && states[i].parentEntityId == parentId) {
            if (i > lastChild) {
                lastChild = i;
            }
        }
    }
    return lastChild;
}

int BondingSystem::findPrunableLeaf(int parentId, const std::vector<StateComponent>& states) {
    // Buscamos un átomo que tenga a parentId en su linaje (o sea el hijo directo)
    // pero que él mismo NO tenga hijos.
    
    int bestLeaf = -1;
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isClustered && states[i].parentEntityId != -1) {
            // Verificamos si este átomo i pertenece a la estructura del parentId
            // (Simplificado: buscamos hijos directos de parentId que sean hojas, 
            // o recursivamente buscamos la hoja más lejana)
                if (states[i].parentEntityId == parentId) {
                    // Es hijo directo. Buscamos si él mismo tiene hijos
                    bool hasChildren = false;
                    for (int j = 0; j < (int)states.size(); j++) {
                        if (states[j].isClustered && states[j].parentEntityId == i) {
                            hasChildren = true;
                            break;
                        }
                    }
                    
                    if (!hasChildren) {
                        // Es una hoja directa. Preferimos la de mayor índice (más reciente)
                        if (i > bestLeaf) bestLeaf = i;
                    } else {
                        // Si tiene hijos, buscamos recursivamente en esa rama la hoja más profunda
                        int leafInBranch = findPrunableLeaf(i, states);
                        if (leafInBranch > bestLeaf) bestLeaf = leafInBranch;
                    }
                }
        }
    }
    return bestLeaf;
}

void BondingSystem::breakAllBonds(int entityId, std::vector<StateComponent>& states, 
                                  std::vector<AtomComponent>& atoms) {
    if (entityId < 0 || entityId >= (int)states.size()) return;

    // 1. Romper conexión con el padre
    if (states[entityId].parentEntityId != -1) {
        breakBond(entityId, states, atoms);
    }

    // 2. Romper todas las conexiones con los hijos
    // Iteramos al revés para evitar problemas de índice si cambiara el tamaño (que no cambia)
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i].isClustered && states[i].parentEntityId == entityId) {
            breakBond(i, states, atoms);
        }
    }

    TraceLog(LOG_INFO, "[BOND] Full Isolation: Atomo %d liberado de todos sus enlaces", entityId);
}
