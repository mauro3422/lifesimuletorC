#include "Player.hpp"
#include "DockingSystem.hpp"
#include "../input/InputHandler.hpp"
#include "../physics/SpatialGrid.hpp"
#include "../core/Config.hpp"
#include "../core/MathUtils.hpp"
#include <cmath>

Player::Player(int entityIndex) : playerIndex(entityIndex) {
    atomicNumber = 1; 
    speed = Config::PLAYER_SPEED;
    lastRootId = -1;
}

void Player::update(float dt, const InputHandler& input, 
                    std::vector<TransformComponent>& worldTransforms, 
                    const Camera2D& camera,
                    const SpatialGrid& grid,
                    std::vector<StateComponent>& states,
                    std::vector<AtomComponent>& atoms) {
    
    auto& transform = worldTransforms[playerIndex];

    // 1. MOVEMENT (smooth acceleration)
    Vector2 dir = input.getMovementDirection();
    if (dir.x != 0 || dir.y != 0) {
        float targetVx = dir.x * speed;
        float targetVy = dir.y * speed;
        transform.vx += (targetVx - transform.vx) * Config::PLAYER_ACCEL; 
        transform.vy += (targetVy - transform.vy) * Config::PLAYER_ACCEL;
    } else {
        transform.vx *= Config::DRAG_COEFFICIENT;
        transform.vy *= Config::DRAG_COEFFICIENT;
    }

    // 2. THERMODYNAMIC JITTER
    transform.vx += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
    transform.vy += MathUtils::getJitter() * Config::THERMODYNAMIC_JITTER;
    transform.x += transform.vx * dt;
    transform.y += transform.vy * dt;

    // 3. TRACTOR BEAM UPDATE
    int lastTarget = tractor.getTargetIndex();
    Vector2 mouseWorld = GetScreenToWorld2D(input.getMousePosition(), camera);
    tractor.update(mouseWorld, input.isTractorBeamActive(), worldTransforms, states, atoms, grid);
    
    int currentTarget = tractor.getTargetIndex();
    int currentRoot = (currentTarget != -1) ? MathUtils::findMoleculeRoot(currentTarget, states) : -1;

    // DETECT CHANGE: Target changed OR Root changed (due to bond breaks while dragging)
    if (lastTarget != -1 && (currentTarget != lastTarget || currentRoot != lastRootId)) {
        if (lastRootId != -1 && lastRootId < (int)states.size()) {
            // FIX: Clear shield for EVERY member of the MOLECULE associated with the OLD root
            std::vector<int> members = MathUtils::getMoleculeMembers(lastRootId, states);
            for (int mIdx : members) {
                states[mIdx].isShielded = false;
            }
            // Explicitly clear old root just in case getMoleculeMembers didn't catch it
            states[lastRootId].isShielded = false; 
        }
    }
    lastRootId = currentRoot;

    // 4. AUTO-DOCKING (delegated to DockingSystem)
    if (tractor.isActive()) {
        int targetIdx = tractor.getTargetIndex();
        if (DockingSystem::tryAutoDock(targetIdx, playerIndex, states, atoms, 
                                        worldTransforms, undoManager.getAttachmentOrder())) {
            tractor.release();
        }
    }

    // 5. UNDO (delegated to UndoManager)
    if (input.isReleaseTriggered()) {
        undoManager.undoLast(playerIndex, states, atoms);
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

    // Break bonds on first capture (Phase 43 fix: also check ring/cycle bonds)
    if (tractor.becameActive()) {
        bool hasBonds = states[idx].parentEntityId != -1 ||
                        states[idx].cycleBondId != -1 ||
                        states[idx].isInRing ||
                        states[idx].isClustered ||  // Also check if clustered at all
                        BondingSystem::findLastChild(idx, states) != -1;
        if (hasBonds) {
            // Get old members BEFORE breaking (to re-propagate them after)
            std::vector<int> oldMembers = MathUtils::getMoleculeMembers(idx, states);
            
            BondingSystem::breakAllBonds(idx, states, atoms);
            targetIdx = idx;  // Now isolated, it's its own root
            
            // Re-propagate moleculeId for remaining structure members
            for (int oldId : oldMembers) {
                if (oldId != idx && states[oldId].isClustered) {
                    BondingSystem::propagateMoleculeId(oldId, states);
                }
            }
        }
    }
    
    // Only shield the ISOLATED atom now (not the whole old molecule)
    states[targetIdx].isShielded = true;
    auto& targetTr = worldTransforms[targetIdx];
    Vector2 tPos = tractor.getTargetPosition();
    
    float dx = tPos.x - targetTr.x;
    float dy = tPos.y - targetTr.y;
    float dist = MathUtils::dist(tPos, Vector2{targetTr.x, targetTr.y});

    if (dist > 5.0f) {
        targetTr.vx *= Config::TRACTOR_DAMPING; 
        targetTr.vy *= Config::TRACTOR_DAMPING;

        float speedFactor = 1.0f;
        float brakeThreshold = Config::TRACTOR_REACH_MIN * 1.5f;
        if (dist < brakeThreshold) {
            speedFactor = (dist - Config::TRACTOR_REACH_MIN) / (brakeThreshold - Config::TRACTOR_REACH_MIN);
            if (speedFactor < 0.1f) speedFactor = 0.1f;
        }

        Vector2 dir = MathUtils::normalize(Vector2{dx, dy});
        float steerX = dir.x * Config::TRACTOR_MAX_SPEED * speedFactor;
        float steerY = dir.y * Config::TRACTOR_MAX_SPEED * speedFactor;
        
        float jitterMag = (1.0f - (dist / Config::TRACTOR_JITTER_GRADIENT)) * Config::TRACTOR_JITTER_INTENSITY;
        if (jitterMag < 0) jitterMag = 0;
        
        targetTr.vx += (steerX - targetTr.vx) * Config::TRACTOR_STEER_FACTOR + (MathUtils::getJitter() * jitterMag);
        targetTr.vy += (steerY - targetTr.vy) * Config::TRACTOR_STEER_FACTOR + (MathUtils::getJitter() * jitterMag);
    } else {
        targetTr.vx *= Config::TRACTOR_HOLD_DAMPING;
        targetTr.vy *= Config::TRACTOR_HOLD_DAMPING;
    }
}
