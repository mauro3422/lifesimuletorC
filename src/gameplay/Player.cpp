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
                    std::vector<AtomComponent>& atoms) {
    
    auto& transform = worldTransforms[playerIndex];

    // 1. MOVIMIENTO SUAVE
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

    // 2. VIBRACIÓN
    transform.vx += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
    transform.vy += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
    transform.x += transform.vx * dt;
    transform.y += transform.vy * dt;

    // 3. TRACTOR BEAM UPDATE
    int lastTarget = tractor.getTargetIndex();
    Vector2 mouseWorld = GetScreenToWorld2D(input.getMousePosition(), camera);
    tractor.update(mouseWorld, input.isTractorBeamActive(), worldTransforms, grid);
    
    int currentTarget = tractor.getTargetIndex();
    if (lastTarget != -1 && lastTarget != currentTarget) {
        if (lastTarget < (int)states.size()) {
            states[lastTarget].isShielded = false;
        }
    }

    // 4. AUTO-DOCKING
    if (tractor.isActive()) {
        int targetIdx = tractor.getTargetIndex();
        if (targetIdx != -1 && targetIdx != playerIndex) {
            if (!states[targetIdx].isClustered) {
                float dx = transform.x - worldTransforms[targetIdx].x;
                float dy = transform.y - worldTransforms[targetIdx].y;
                float dist = std::sqrt(dx*dx + dy*dy);

                if (dist < Config::TRACTOR_DOCKING_RANGE * 1.2f) {
                    if (BondingSystem::tryBond(targetIdx, playerIndex, states, atoms, worldTransforms, true) == BondingSystem::SUCCESS) {
                        int root = MathUtils::findMoleculeRoot(targetIdx, states);
                        states[root].isShielded = false;
                        attachmentOrder.push_back(targetIdx); 
                        NotificationManager::getInstance().show("¡Acoplado!", Config::THEME_SUCCESS);
                        tractor.release();
                    }
                }
            }
        }
    }

    // 5. DESACOPLAMIENTO MANUAL (Botón Derecho)
    if (input.isReleaseTriggered()) {
        if (states[playerIndex].parentEntityId != -1) {
            BondingSystem::breakBond(playerIndex, states, atoms);
            NotificationManager::getInstance().show("Jugador liberado", Config::THEME_INFO);
            return;
        }

        bool removed = false;
        while (!attachmentOrder.empty()) {
            int candidate = attachmentOrder.back();
            attachmentOrder.pop_back();
            if (states[candidate].isClustered && states[candidate].parentEntityId != -1) {
                if (MathUtils::findMoleculeRoot(candidate, states) == playerIndex) {
                    BondingSystem::breakBond(candidate, states, atoms);
                    NotificationManager::getInstance().show("Undo: Atomo liberado", Config::THEME_INFO);
                    removed = true;
                    break;
                }
            }
        }

        if (!removed) {
            int leafId = BondingSystem::findPrunableLeaf(playerIndex, states);
            if (leafId != -1) {
                BondingSystem::breakBond(leafId, states, atoms);
                NotificationManager::getInstance().show("Hoja podada", Config::THEME_INFO);
            }
        }
    }
}

void Player::applyPhysics(std::vector<TransformComponent>& worldTransforms,
                          std::vector<StateComponent>& states,
                          std::vector<AtomComponent>& atoms) {
    
    if (!tractor.isActive()) return;

    int idx = tractor.getTargetIndex();
    if (idx < 0 || idx >= (int)worldTransforms.size() || idx == playerIndex) return;

    int targetIdx = MathUtils::findMoleculeRoot(idx, states);
    
    if (targetIdx == playerIndex) {
        tractor.release();
        return;
    }

    if (tractor.becameActive()) {
        if (states[idx].parentEntityId != -1 || BondingSystem::findLastChild(idx, states) != -1) {
            BondingSystem::breakAllBonds(idx, states, atoms);
            targetIdx = idx;
        }
    }
    
    states[targetIdx].isShielded = true;
    auto& targetTr = worldTransforms[targetIdx];
    Vector2 tPos = tractor.getTargetPosition();
    
    float dx = tPos.x - targetTr.x;
    float dy = tPos.y - targetTr.y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist > 5.0f) {
        targetTr.vx *= Config::TRACTOR_DAMPING; 
        targetTr.vy *= Config::TRACTOR_DAMPING;

        float speedFactor = 1.0f;
        float brakeThreshold = Config::TRACTOR_REACH_MIN * 1.5f;
        if (dist < brakeThreshold) {
            speedFactor = (dist - Config::TRACTOR_REACH_MIN) / (brakeThreshold - Config::TRACTOR_REACH_MIN);
            if (speedFactor < 0.1f) speedFactor = 0.1f;
        }

        float steerX = (dx / dist) * Config::TRACTOR_MAX_SPEED * speedFactor;
        float steerY = (dy / dist) * Config::TRACTOR_MAX_SPEED * speedFactor;
        
        float jitterMag = (1.0f - (dist / Config::TRACTOR_JITTER_GRADIENT)) * Config::TRACTOR_JITTER_INTENSITY;
        if (jitterMag < 0) jitterMag = 0;
        
        targetTr.vx += (steerX - targetTr.vx) * Config::TRACTOR_STEER_FACTOR + (MathUtils::getJitter() * jitterMag);
        targetTr.vy += (steerY - targetTr.vy) * Config::TRACTOR_STEER_FACTOR + (MathUtils::getJitter() * jitterMag);
    } else {
        targetTr.vx *= Config::TRACTOR_HOLD_DAMPING;
        targetTr.vy *= Config::TRACTOR_HOLD_DAMPING;
    }
}
