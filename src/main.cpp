#include "raylib.h"
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cstdarg>

// Arquitectura Modular
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
#include <iostream>

// File Logger para persistencia de logs
static FILE* logFile = nullptr;
void FileLogCallback(int logLevel, const char* text, va_list args) {
    if (logFile) {
        const char* levelStr[] = { "ALL", "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE" };
        fprintf(logFile, "[%s] ", levelStr[logLevel]);
        vfprintf(logFile, text, args);
        fprintf(logFile, "\n");
        fflush(logFile);
    }
    // También mostrar en consola
    char buffer[256];
    vsnprintf(buffer, 256, text, args);
    printf("[%s] %s\n", logLevel == LOG_INFO ? "INFO" : "DEBUG", buffer);
}

int main() {
    // Abrir archivo de log ANTES de inicializar Raylib
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

    PhysicsEngine physics;
    
    // 0. INICIALIZAR ZONAS AMBIENTALES (ISLAS)
    auto clayIsland = std::make_shared<ClayZone>((Rectangle){ -1500, -800, 2000, 1600 });
    physics.getEnvironment().addZone(clayIsland);
    InputHandler input;
    Inspector inspector;
    CameraSystem cameraSys; 
    // 1. PANTALLA DE CARGA Y SECUENCIA DE INICIALIZACIÓN
    LoadingScreen loading;
    
    // Paso 1: Base de Datos Química
    loading.draw(0.2f, "Sintetizando tabla periódica...");
    ChemistryDatabase& db = ChemistryDatabase::getInstance(); 
    
    // Paso 2: Generación del Mundo (Densidad Primordial)
    loading.draw(0.5f, "Instanciando sopa primordial (1000 átomos)...");
    World world;
    world.initialize();

    // Paso 3: Misiones y Gameplay
    loading.draw(0.8f, "Configurando protocolos de biogénesis...");
    MissionManager::getInstance().initialize();
    
    // Paso 4: Finalización
    loading.draw(1.0f, "Listo para la simulación.");
    
    Quimidex quimidex;

    // 2. INICIALIZAR PLAYER
    Player player(0); 

    // 3. CONFIGURAR CÁMARA
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
        
        accumulator += frameTime;

        input.resetFrameState();
        input.update();

        // SIMULACION (Fixed Timestep)
        while (accumulator >= fixedDeltaTime) {
            player.update(fixedDeltaTime, input, world.transforms, camera, physics.getGrid(), world.states, world.atoms);
            player.applyPhysics(world.transforms, world.states, world.atoms);
            physics.step(fixedDeltaTime, world.transforms, world.atoms, world.states);
            BondingSystem::updateHierarchy(world.transforms, world.states, world.atoms);
            BondingSystem::updateSpontaneousBonding(world.states, world.atoms, world.transforms, &physics.getEnvironment(), player.getTractor().getTargetIndex());
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
                // Estado 1: Abrir Inspector de Atomo
                inspectingPlayer = true;
                inspectingMolecule = false;
                selectedEntityIndex = -1;
                inspector.setMolecule(nullptr);
            } else if (inspectingPlayer) {
                // Estado 2: Pasar a Inspector de Molécula
                inspectingPlayer = false;
                inspectingMolecule = true;
            } else {
                // Estado 3: Cerrar todo
                inspectingPlayer = false;
                inspectingMolecule = false;
            }
        }

        if (IsKeyPressed(KEY_Q)) {
            quimidex.toggle();
        }

        if (inspectingMolecule) {
            int targetIdx = player.getTractor().getTargetIndex();
            // Fallback: Si no tenemos target, inspeccionamos la molécula del jugador (Entity 0)
            if (targetIdx == -1) targetIdx = 0; 

            if (targetIdx != -1 && targetIdx < (int)world.atoms.size()) {
                auto composition = MathUtils::scanMoleculeComposition(targetIdx, world.states, world.atoms);
                const Molecule* detected = ChemistryDatabase::getInstance().findMoleculeByComposition(composition);
                inspector.setMolecule(detected);
                inspector.setComposition(composition);
                
                if (detected) {
                    DiscoveryLog::getInstance().discoverMolecule(detected->id);
                    MissionManager::getInstance().notifyMoleculeDiscovered(detected->name);
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
            EndMode2D();

            HUD::draw(camera, cameraSys.getMode() == CameraSystem::FREE_LOOK, input);

            if (inspectingPlayer) {
                // El jugador es siempre la entidad 0
                inspector.draw(db.getElement(world.atoms[0].atomicNumber), 0, input, world.states, world.atoms);
            } else if (inspectingMolecule || (selectedEntityIndex != -1 && selectedEntityIndex < (int)world.atoms.size())) {
                int entityToInspect = inspectingMolecule ? player.getTractor().getTargetIndex() : selectedEntityIndex;
                if (entityToInspect == -1 && inspectingMolecule) entityToInspect = 0; // Fallback al jugador

                if (entityToInspect != -1 && entityToInspect < (int)world.atoms.size()) {
                    int rootIdx = MathUtils::findMoleculeRoot(entityToInspect, world.states);
                    if (rootIdx != -1) {
                        inspector.draw(db.getElement(world.atoms[rootIdx].atomicNumber), rootIdx, input, world.states, world.atoms);
                    }
                }
            }

            // NOTIFICACIONES (encima de todo)
            NotificationManager::getInstance().draw();
            quimidex.draw(input);

        EndDrawing();
    }

    CloseWindow();
    if (logFile) fclose(logFile);
    return 0;
}

