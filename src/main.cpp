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
#include "gameplay/Player.hpp"
#include "ui/LabelSystem.hpp"
#include "ui/Inspector.hpp"
#include "ui/HUD.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/NotificationManager.hpp"

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
    SetTargetFPS(144); 

    PhysicsEngine physics;
    InputHandler input;
    Inspector inspector;
    CameraSystem cameraSys; 
    ChemistryDatabase& db = ChemistryDatabase::getInstance();
    
    // 1. INICIALIZAR MUNDO (ECS ENCAPSULADO)
    World world;
    world.initialize();

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
            BondingSystem::updateSpontaneousBonding(world.states, world.atoms, world.transforms, player.getTractor().getTargetIndex());
            NotificationManager::getInstance().update(fixedDeltaTime);
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
            inspectingPlayer = true;
            selectedEntityIndex = -1;
            inspectingMolecule = false;
        }

        if (input.isSpaceDoubleTriggered()) {
            inspectingMolecule = true;
            inspectingPlayer = false;
        }

        BeginDrawing();
            ClearBackground(Config::THEME_BACKDROP); 

            BeginMode2D(camera);
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
                inspector.draw(db.getElement(world.atoms[0].atomicNumber), 0, input);
            } else if (inspectingMolecule) {
                Rectangle panelRect = { (float)Config::MOL_VIEW_X, (float)GetScreenHeight() - Config::MOL_VIEW_Y_OFFSET, (float)Config::MOL_VIEW_WIDTH, (float)Config::MOL_VIEW_HEIGHT };
                UIWidgets::drawPanel(panelRect, input, Config::THEME_ACCENT);
                UIWidgets::drawHeader(panelRect, "MOLECULE ANALYZER", Config::THEME_ACCENT);
                DrawText("Active Molecular Cluster", (int)panelRect.x + 10, (int)panelRect.y + 30, 12, WHITE);
            } else if (selectedEntityIndex != -1 && selectedEntityIndex < (int)world.atoms.size()) {
                inspector.draw(db.getElement(world.atoms[selectedEntityIndex].atomicNumber), selectedEntityIndex, input);
            }

            // NOTIFICACIONES (encima de todo)
            NotificationManager::getInstance().draw();

        EndDrawing();
    }

    CloseWindow();
    if (logFile) fclose(logFile);
    return 0;
}

