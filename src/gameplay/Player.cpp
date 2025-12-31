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

    // Phase 43 FIX: Always work with the CLICKED atom (idx), not the molecule root
    // On first capture, isolate it. On subsequent frames, just move the already-isolated atom.
    
    // Check if this atom belongs to player's molecule
    int rootCheck = MathUtils::findMoleculeRoot(idx, states);
    if (rootCheck == playerIndex) {
        tractor.release();
        return;
    }

    // Break bonds on first capture (Phase 43 fix: also check ring/cycle bonds)
    // Phase 45: CTRL modifier for structural movement mode
    bool structuralMode = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool isInFrozenStructure = states[idx].isFrozen && states[idx].structureId != -1;
    
    if (tractor.becameActive()) {
        TraceLog(LOG_INFO, "[TRACTOR_DEBUG] === NEW CAPTURE: idx=%d ===", idx);
        TraceLog(LOG_INFO, "[TRACTOR_DEBUG] BEFORE: parent=%d, cycle=%d, molId=%d, clustered=%d, ring=%d, childCount=%d",
                 states[idx].parentEntityId, states[idx].cycleBondId, states[idx].moleculeId,
                 states[idx].isClustered ? 1 : 0, states[idx].isInRing ? 1 : 0, states[idx].childCount);
        
        // Phase 45: Skip bond breaking if CTRL is held and atom is in frozen structure
        if (structuralMode && isInFrozenStructure) {
            TraceLog(LOG_INFO, "[TRACTOR_DEBUG] STRUCTURAL MODE: Moving structure %d as unit", states[idx].structureId);
        } else {
            bool hasBonds = states[idx].parentEntityId != -1 ||
                            states[idx].cycleBondId != -1 ||
                            states[idx].isInRing ||
                            states[idx].isClustered ||
                            BondingSystem::findLastChild(idx, states) != -1;
            
            TraceLog(LOG_INFO, "[TRACTOR_DEBUG] hasBonds=%d", hasBonds ? 1 : 0);
            
            if (hasBonds) {
                // Get old members BEFORE breaking (to re-propagate them after)
                std::vector<int> oldMembers = MathUtils::getMoleculeMembers(idx, states);
                TraceLog(LOG_INFO, "[TRACTOR_DEBUG] oldMembers.size=%d", (int)oldMembers.size());
                
                BondingSystem::breakAllBonds(idx, states, atoms);
                
                // Re-propagate moleculeId for remaining structure members
                for (int oldId : oldMembers) {
                    if (oldId != idx && states[oldId].isClustered) {
                        BondingSystem::propagateMoleculeId(oldId, states);
                    }
                }
            }
            
            TraceLog(LOG_INFO, "[TRACTOR_DEBUG] AFTER: parent=%d, cycle=%d, molId=%d, clustered=%d, ring=%d, childCount=%d",
                     states[idx].parentEntityId, states[idx].cycleBondId, states[idx].moleculeId,
                     states[idx].isClustered ? 1 : 0, states[idx].isInRing ? 1 : 0, states[idx].childCount);
            
            // Verify isolation
            bool isolated = (states[idx].parentEntityId == -1) && 
                            (states[idx].cycleBondId == -1) &&
                            (states[idx].childList.empty());
            TraceLog(LOG_INFO, "[TRACTOR_DEBUG] ISOLATED=%d (childList.size=%d)", 
                     isolated ? 1 : 0, (int)states[idx].childList.size());
        }
    }
    
    // Always shield and move the CLICKED atom (idx), not any root
    states[idx].isShielded = true;
    auto& targetTr = worldTransforms[idx];
    Vector2 tPos = tractor.getTargetPosition();
    
    float dx = tPos.x - targetTr.x;
    float dy = tPos.y - targetTr.y;
    float dist = MathUtils::dist(tPos, Vector2{targetTr.x, targetTr.y});

    // Calculate velocity for the target atom
    float newVx = 0, newVy = 0;
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
        
        newVx = (steerX - targetTr.vx) * Config::TRACTOR_STEER_FACTOR + (MathUtils::getJitter() * jitterMag);
        newVy = (steerY - targetTr.vy) * Config::TRACTOR_STEER_FACTOR + (MathUtils::getJitter() * jitterMag);
        
        targetTr.vx += newVx;
        targetTr.vy += newVy;
    } else {
        targetTr.vx *= Config::TRACTOR_HOLD_DAMPING;
        targetTr.vy *= Config::TRACTOR_HOLD_DAMPING;
    }
    
    // Phase 45: If in structural mode, apply same velocity to ALL atoms in structure
    if (structuralMode && isInFrozenStructure) {
        int structId = states[idx].structureId;
        for (int i = 0; i < (int)worldTransforms.size(); i++) {
            if (i != idx && states[i].structureId == structId && states[i].isFrozen) {
                states[i].isShielded = true;  // Shield all structure atoms
                worldTransforms[i].vx = targetTr.vx;
                worldTransforms[i].vy = targetTr.vy;
            }
        }
    }
}
