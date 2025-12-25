#include "Player.hpp"
#include "../input/InputHandler.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../physics/BondingSystem.hpp"
#include "../core/Config.hpp"
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
    
    // Obtenemos referencia directa al transform del jugador en el mundo
    auto& transform = worldTransforms[playerIndex];

    // 1. MOVIMIENTO SUAVE (Steering)
    Vector2 dir = input.getMovementDirection();
    if (dir.x != 0 || dir.y != 0) {
        float targetVx = dir.x * speed;
        float targetVy = dir.y * speed;
        
        // Aceleración suave hacia la velocidad objetivo
        transform.vx += (targetVx - transform.vx) * Config::PLAYER_ACCEL; 
        transform.vy += (targetVy - transform.vy) * Config::PLAYER_ACCEL;
    } else {
        // Frenado inercial
        transform.vx *= Config::PLAYER_FRICTION;
        transform.vy *= Config::PLAYER_FRICTION;
    }

    // 2. VIBRACIÓN TERMODINÁMICA (Browniana del Jugador)
    transform.vx += (float)GetRandomValue(-100, 100) / 100.0f * Config::THERMODYNAMIC_JITTER;
    transform.vy += (float)GetRandomValue(-100, 100) / 100.0f * Config::THERMODYNAMIC_JITTER;

    // Actualizamos posición (Integración simple para el jugador antes de la física global)
    transform.x += transform.vx * dt;
    transform.y += transform.vy * dt;

    // 3. HERRAMIENTAS: Tractor Beam
    Vector2 mouseWorld = GetScreenToWorld2D(input.getMousePosition(), camera);
    tractor.update(mouseWorld, input.isTractorBeamActive(), worldTransforms, grid);

    // 4. AUTO-DOCKING (Ensamblador Atómico)
    if (tractor.isActive()) {
        int targetIdx = tractor.getTargetIndex();
        if (targetIdx != -1 && targetIdx != playerIndex) {
            // Solo intentamos unir atomos LIBRES (no ya enlazados a otra molecula)
            if (!states[targetIdx].isClustered) {
                float dx = transform.x - worldTransforms[targetIdx].x;
                float dy = transform.y - worldTransforms[targetIdx].y;
                float dist = std::sqrt(dx*dx + dy*dy);

                if (dist < Config::TRACTOR_DOCKING_RANGE) {
                    if (BondingSystem::tryBond(targetIdx, playerIndex, states, atoms, worldTransforms)) {
                        TraceLog(LOG_INFO, ">>> [BONDING] Atomo ID %d unido a la molecula del Jugador!", targetIdx);
                        tractor.release();
                    } else {
                        // Notificacion de enlace fallido
                        NotificationManager::getInstance().show("Enlace incompatible!", RED, 1.5f);
                        tractor.release();
                    }
                }
            }
            // Si el atomo esta en otra molecula, atraemos la molecula entera (sin intentar bond)
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
            
            // Si el atomo esta en una molecula, encontramos su raiz para mover toda la molecula
            int targetIdx = idx;
            if (states[idx].isClustered) {
                // Encontrar la raiz de la molecula
                while (states[targetIdx].parentEntityId != -1) {
                    targetIdx = states[targetIdx].parentEntityId;
                }
                // Si la raiz es el jugador, ya esta unido - soltar
                if (targetIdx == playerIndex) {
                    tractor.release();
                    return;
                }
            }
            
            auto& targetTr = worldTransforms[targetIdx];
            float dx = transform.x - targetTr.x;
            float dy = transform.y - targetTr.y;
            float dist = std::sqrt(dx*dx + dy*dy);

            if (dist > Config::TRACTOR_REACH_MIN) {
                targetTr.vx *= Config::TRACTOR_DAMPING; 
                targetTr.vy *= Config::TRACTOR_DAMPING;

                float steerX = (dx / dist) * Config::TRACTOR_MAX_SPEED;
                float steerY = (dy / dist) * Config::TRACTOR_MAX_SPEED;

                float jitterMag = (1.0f - (dist / Config::TRACTOR_JITTER_GRADIENT)) * Config::TRACTOR_JITTER_INTENSITY;
                if (jitterMag < 0) jitterMag = 0;
                
                float jx = (float)GetRandomValue(-100, 100) / 100.0f * jitterMag;
                float jy = (float)GetRandomValue(-100, 100) / 100.0f * jitterMag;

                targetTr.vx += (steerX - targetTr.vx) * Config::TRACTOR_STEER_FACTOR + jx;
                targetTr.vy += (steerY - targetTr.vy) * Config::TRACTOR_STEER_FACTOR + jy;
            } else {
                targetTr.vx *= Config::TRACTOR_HOLD_DAMPING;
                targetTr.vy *= Config::TRACTOR_HOLD_DAMPING;
            }
        }
    }
}
