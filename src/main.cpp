#include "raylib.h"
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cstdarg>

// Modular Architecture
#include "ecs/World.hpp"
#include "physics/PhysicsEngine.hpp"
#include "physics/BondingSystem.hpp"
#include "physics/SpatialGrid.hpp"
#include "rendering/CameraSystem.hpp"
#include "rendering/Renderer25D.hpp"
#include "chemistry/ChemistryDatabase.hpp"
#include "core/Config.hpp"
#include "core/MathUtils.hpp"
#include "gameplay/Player.hpp"
#include "ui/LabelSystem.hpp"
#include "ui/Inspector.hpp"
#include "ui/HUD.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/NotificationManager.hpp"
#include "ui/Quimidex.hpp"
#include "gameplay/MissionManager.hpp"
#include "world/zones/ClayZone.hpp"
#include "ui/LoadingScreen.hpp"
#include "core/LocalizationManager.hpp"
#include <iostream>

// File Logger for persistence
static FILE* logFile = nullptr;
void FileLogCallback(int logLevel, const char* text, va_list args) {
    if (logFile) {
        const char* levelStr[] = { "ALL", "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE" };
        fprintf(logFile, "[%s] ", levelStr[logLevel]);
        vfprintf(logFile, text, args);
        fprintf(logFile, "\n");
        fflush(logFile);
    }
    // Also show in console
    char buffer[256];
    vsnprintf(buffer, 256, text, args);
    printf("[%s] %s\n", logLevel == LOG_INFO ? "INFO" : "DEBUG", buffer);
}

int main() {
    // Open log file BEFORE initializing Raylib
    logFile = fopen("session.log", "w");
    if (logFile) {
        fprintf(logFile, "=== LIFE SIMULATOR SESSION LOG ===\n");
        fflush(logFile);
    }
    SetTraceLogCallback(FileLogCallback);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, "LifeSimulator C++ | Nano-HD Architecture");
    if (IsWindowReady()) SetWindowMinSize(Config::WINDOW_MIN_WIDTH, Config::WINDOW_MIN_HEIGHT);
    SetTargetFPS(60); 

    // 0. LOCALIZATION INITIALIZATION (Default to Spanish)
    LocalizationManager::getInstance().setLanguage("es");

    PhysicsEngine physics;
    
    // 0. INITIALIZE ENVIRONMENTAL ZONES (ISLANDS)
    // Clay Zone: Offset to the left, acting as a catalyst station (800x800)
    auto clayIsland = std::make_shared<ClayZone>((Rectangle){ -1200, -400, 800, 800 });
    physics.getEnvironment().addZone(clayIsland);
    InputHandler input;
    Inspector inspector;
    CameraSystem cameraSys; 
    
    // 1. LOADING SCREEN & INITIALIZATION SEQUENCE
    LoadingScreen loading;
    LocalizationManager& lang = LocalizationManager::getInstance();
    
    // Step 1: Chemical Database
    loading.draw(0.2f, lang.get("ui.loading.periodic_table").c_str());
    ChemistryDatabase& db = ChemistryDatabase::getInstance(); 
    
    // Step 2: World Generation (Primordial Density)
    loading.draw(0.5f, lang.get("ui.loading.world_gen").c_str());
    World world;
    // TEMPORARY: Using test mode for ring formation debugging
    world.initializeTestMode(); // Change back to world.initialize() when done testing

    // Step 3: Missions and Gameplay
    loading.draw(0.8f, lang.get("ui.loading.missions").c_str());
    MissionManager::getInstance().initialize();
    
    // Step 4: Finalization
    loading.draw(1.0f, lang.get("ui.loading.ready").c_str());
    
    Quimidex quimidex;

    // 2. INITIALIZE PLAYER
    Player player(0); 

    // 3. CONFIGURE CAMERA
    Camera2D camera = { 0 };
    camera.target = { world.transforms[0].x, world.transforms[0].y };
    camera.offset = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
    camera.zoom = Config::CAMERA_INITIAL_ZOOM;

    int selectedEntityIndex = -1;
    bool inspectingPlayer = false;
    bool inspectingMolecule = false;

    float accumulator = 0.0f;
    const float fixedDeltaTime = Config::FIXED_DELTA_TIME; 

    while (!WindowShouldClose()) {
        float frameTime = GetFrameTime();
        if (frameTime > Config::MAX_FRAME_TIME) frameTime = Config::MAX_FRAME_TIME;
        
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
        
        if (IsKeyPressed(KEY_F1)) {
            auto& lm = LocalizationManager::getInstance();
            std::string nextLang = (lm.getLanguageCode() == "es") ? "en" : "es";
            lm.setLanguage(nextLang);
            
            // Reload all localized systems
            db.reload();
            MissionManager::getInstance().reload();
            quimidex.reload();
            
            NotificationManager::getInstance().show(
                (nextLang == "es") ? "Idioma: ESPAÑOL" : "Language: ENGLISH",
                LIME
            );
        }

        accumulator += frameTime;

        input.resetFrameState();
        input.update();

        // SIMULATION (Fixed Timestep)
        while (accumulator >= fixedDeltaTime) {
            player.update(fixedDeltaTime, input, world.transforms, camera, physics.getGrid(), world.states, world.atoms);
            player.applyPhysics(world.transforms, world.states, world.atoms);
            physics.step(fixedDeltaTime, world.transforms, world.atoms, world.states);
            BondingSystem::updateHierarchy(world.transforms, world.states, world.atoms);
            BondingSystem::updateSpontaneousBonding(world.states, world.atoms, world.transforms, physics.getGrid(), &physics.getEnvironment(), player.getTractor().getTargetIndex());
            NotificationManager::getInstance().update(fixedDeltaTime);
            MissionManager::getInstance().update(fixedDeltaTime);
            accumulator -= fixedDeltaTime;
        }

        // VISUALS
        camera.offset = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
        cameraSys.update(camera, input, { world.transforms[0].x, world.transforms[0].y }, frameTime);

        if (input.isSelectionTriggered()) {
            selectedEntityIndex = player.getTractor().getTargetIndex();
            inspectingPlayer = false;
            inspectingMolecule = false;
        }

        if (input.isSpaceTriggered()) {
            if (!inspectingPlayer && !inspectingMolecule) {
                // State 1: Open Atom Inspector
                inspectingPlayer = true;
                inspectingMolecule = false;
                selectedEntityIndex = -1;
                inspector.setMolecule(nullptr);
            } else if (inspectingPlayer) {
                // State 2: Switch to Molecule Inspector
                inspectingPlayer = false;
                inspectingMolecule = true;
            } else {
                // State 3: Close all
                inspectingPlayer = false;
                inspectingMolecule = false;
            }
        }

        if (IsKeyPressed(KEY_Q)) {
            quimidex.toggle();
        }

        if (inspectingMolecule) {
            int targetIdx = player.getTractor().getTargetIndex();
            if (targetIdx == -1) targetIdx = 0; // Fallback to player molecule

            if (targetIdx >= 0 && targetIdx < (int)world.atoms.size()) {
                auto composition = MathUtils::getMoleculeComposition(targetIdx, world.states, world.atoms);
                const Molecule* detected = ChemistryDatabase::getInstance().findMoleculeByComposition(composition);
                
                inspector.setMolecule(detected);
                inspector.setComposition(composition);
                
                if (detected) {
                    DiscoveryLog::getInstance().discoverMolecule(detected->id);
                    MissionManager::getInstance().notifyMoleculeDiscovered(detected->id);
                }
            }
        }

        BeginDrawing();
            ClearBackground(Config::THEME_BACKDROP); 

            BeginMode2D(camera);
                physics.getEnvironment().draw();
                Renderer25D::drawAtoms(world.transforms, world.atoms, world.states);
                LabelSystem::draw(camera, world.transforms, world.atoms);

                if (player.getTractor().isActive() && player.getTractor().getTargetIndex() != -1) {
                    TransformComponent& targetTr = world.transforms[player.getTractor().getTargetIndex()];
                    DrawLineEx({world.transforms[0].x, world.transforms[0].y}, {targetTr.x, targetTr.y}, Config::TRACTOR_BEAM_WIDTH, Fade(Config::THEME_BORDER, 0.6f));
                    DrawCircleLines((int)targetTr.x, (int)targetTr.y, Config::TRACTOR_TARGET_CIRCLE, Config::THEME_BORDER);
                }
                
                // DEBUG: Draw Slots for Selected Entity (Player or Tractor Target)
                if (inspectingPlayer) {
                    Renderer25D::drawDebugSlots(0, world.transforms, world.atoms);
                } else if (inspectingMolecule || selectedEntityIndex != -1) {
                    int target = (selectedEntityIndex != -1) ? selectedEntityIndex : player.getTractor().getTargetIndex();
                    if (target != -1) {
                        Renderer25D::drawDebugSlots(target, world.transforms, world.atoms);
                    }
                }
            EndMode2D();

            HUD::draw(camera, cameraSys.getMode() == CameraSystem::FREE_LOOK, input);

            if (inspectingPlayer) {
                // Player is always entity 0
                inspector.draw(db.getElement(world.atoms[0].atomicNumber), 0, input, world.states, world.atoms);
            } else if (inspectingMolecule || (selectedEntityIndex != -1 && selectedEntityIndex < (int)world.atoms.size())) {
                int entityToInspect = inspectingMolecule ? player.getTractor().getTargetIndex() : selectedEntityIndex;
                if (entityToInspect == -1 && inspectingMolecule) entityToInspect = 0; // Fallback to player

                if (entityToInspect != -1 && entityToInspect < (int)world.atoms.size()) {
                    int rootIdx = MathUtils::findMoleculeRoot(entityToInspect, world.states);
                    if (rootIdx != -1) {
                        inspector.draw(db.getElement(world.atoms[rootIdx].atomicNumber), rootIdx, input, world.states, world.atoms);
                    }
                }
            }

            // NOTIFICATIONS (Above all)
            NotificationManager::getInstance().draw();
            quimidex.draw(input);

        EndDrawing();
    }

    CloseWindow();
    if (logFile) fclose(logFile);
    return 0;
}


