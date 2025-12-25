#include "Player.hpp"
#include "../input/InputHandler.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../physics/BondingSystem.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include "../ui/NotificationManager.hpp"
#include <cmath>
#include <vector>

Player::Player(int entityIndex) : playerIndex(entityIndex) {
    atomicNumber = 1; 
    speed = Config::PLAYER_SPEED;
}

void Player::update(float dt, const InputHandler& input, 
                    std::vector<TransformComponent>& worldTransforms, 
                    const Camera2D& camera,
                    const SpatialGrid& grid,
                    std::vector<StateComponent>& states,
                    const std::vector<AtomComponent>& atoms) {
    
    auto& transform = worldTransforms[playerIndex];

    // 1. MOVIMIENTO SUAVE (Steering)
    Vector2 dir = input.getMovementDirection();
    if (dir.x != 0 || dir.y != 0) {
        float targetVx = dir.x * speed;
        float targetVy = dir.y * speed;
        
        transform.vx += (targetVx - transform.vx) * Config::PLAYER_ACCEL; 
        transform.vy += (targetVy - transform.vy) * Config::PLAYER_ACCEL;
    } else {
        transform.vx *= Config::PLAYER_FRICTION;
        transform.vy *= Config::PLAYER_FRICTION;
    }

    // 2. VIBRACIÓN TERMODINÁMICA (Browniana)
    transform.vx += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
    transform.vy += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;

    transform.x += transform.vx * dt;
    transform.y += transform.vy * dt;

    // 3. HERRAMIENTAS: Tractor Beam
    Vector2 mouseWorld = GetScreenToWorld2D(input.getMousePosition(), camera);
    tractor.update(mouseWorld, input.isTractorBeamActive(), worldTransforms, grid);

    // 4. AUTO-DOCKING (Ensamblador Atómico)
    if (tractor.isActive()) {
        int targetIdx = tractor.getTargetIndex();
        if (targetIdx != -1 && targetIdx != playerIndex) {
            if (!states[targetIdx].isClustered) {
                float dx = transform.x - worldTransforms[targetIdx].x;
                float dy = transform.y - worldTransforms[targetIdx].y;
                float dist = std::sqrt(dx*dx + dy*dy);

                if (dist < Config::TRACTOR_DOCKING_RANGE) {
                    // MODO FORZADO: El jugador ignora el angulo y busca el slot libre mas cercano
                    BondingSystem::BondError error = BondingSystem::tryBond(targetIdx, playerIndex, states, atoms, worldTransforms, true);
                    
                    if (error == BondingSystem::SUCCESS) {
                        NotificationManager::getInstance().show("¡Acoplado a la molécula del Jugador!", Config::THEME_SUCCESS);
                        tractor.release();
                    } else if (error == BondingSystem::VALENCY_FULL) {
                        NotificationManager::getInstance().show("¡Estructura Molecular Saturada!", Config::THEME_WARNING);
                        tractor.release();
                    }
                }
            }
        }
    }
}

void Player::applyPhysics(std::vector<TransformComponent>& worldTransforms,
                          std::vector<StateComponent>& states,
                          const std::vector<AtomComponent>& atoms) {
    
    auto& transform = worldTransforms[playerIndex];
    
    if (tractor.isActive()) {
        int idx = tractor.getTargetIndex();
        if (idx >= 0 && idx < (int)worldTransforms.size() && idx != playerIndex) {
            
            int targetIdx = MathUtils::findMoleculeRoot(idx, states);
            
            // Si la raiz es el jugador, ya esta unido - soltar
            if (targetIdx == playerIndex) {
                tractor.release();
                return;
            }

            // --- LÓGICA DE RUPTURA DE ENLACES ---
            // Si el átomo capturado tiene un padre (está enlazado), lo liberamos
            // para que pueda ser atraído y eventualmente unirse al jugador.
            if (states[idx].parentEntityId != -1) {
                TraceLog(LOG_INFO, "[TRACTOR] Rompiendo enlace del atomo %d para captura", idx);
                states[idx].parentEntityId = -1;
                states[idx].isClustered = false;
                states[idx].moleculeId = -1;
                // Al liberarlo, targetIdx ahora es simplemente idx
                targetIdx = idx;
            }
            
            auto& targetTr = worldTransforms[targetIdx];
            float dx = transform.x - targetTr.x;
            float dy = transform.y - targetTr.y;
            float dist = std::sqrt(dx*dx + dy*dy);

            if (dist > Config::TRACTOR_REACH_MIN) {
                // Amortiguación dinámica: Cuanto más cerca, más frenamos la velocidad lateral
                float currentDamping = Config::TRACTOR_DAMPING;
                if (dist < Config::TRACTOR_REACH_MIN * 3.0f) {
                    currentDamping *= 0.85f; // Frenado extra cerca del jugador
                }
                
                targetTr.vx *= currentDamping; 
                targetTr.vy *= currentDamping;

                // FRENADO PROGRESIVO: Si está cerca, reducimos la potencia de atracción drásticamente
                float speedFactor = 1.0f;
                float brakeThreshold = Config::TRACTOR_REACH_MIN * 2.5f;
                if (dist < brakeThreshold) {
                    speedFactor = (dist - Config::TRACTOR_REACH_MIN) / (brakeThreshold - Config::TRACTOR_REACH_MIN);
                    if (speedFactor < 0.01f) speedFactor = 0.01f; // Mínima fuerza para mantenerlo centrado
                }

                float steerX = (dx / dist) * Config::TRACTOR_MAX_SPEED * speedFactor;
                float steerY = (dy / dist) * Config::TRACTOR_MAX_SPEED * speedFactor;

                float jitterMag = (1.0f - (dist / Config::TRACTOR_JITTER_GRADIENT)) * Config::TRACTOR_JITTER_INTENSITY;
                if (jitterMag < 0) jitterMag = 0;
                
                float jx = MathUtils::getJitter() * jitterMag;
                float jy = MathUtils::getJitter() * jitterMag;

                targetTr.vx += (steerX - targetTr.vx) * Config::TRACTOR_STEER_FACTOR + jx;
                targetTr.vy += (steerY - targetTr.vy) * Config::TRACTOR_STEER_FACTOR + jy;
            } else {
                targetTr.vx *= Config::TRACTOR_HOLD_DAMPING;
                targetTr.vy *= Config::TRACTOR_HOLD_DAMPING;
            }
        }
    }
}
