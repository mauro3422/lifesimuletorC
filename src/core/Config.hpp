#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "raylib.h"

namespace Config {
    // --- PHYSICS CONSTANTS ---
    inline constexpr float DRAG_COEFFICIENT = 0.99f;
    inline constexpr float WORLD_BOUNCE = -0.5f;
    inline constexpr float BOND_COMPRESSION = 1.4f; // Separacion para ver la estructura
    inline constexpr float BOND_SNAP_THRESHOLD = 0.45f;
    inline constexpr float BOND_AUTO_RANGE = 30.0f; // Rango para bonding espontaneo NPC

    // --- WORLD DIMENSIONS ---
    inline constexpr int WORLD_WIDTH_MIN = -5000;
    const int WORLD_WIDTH_MAX = 5000;
    const int WORLD_HEIGHT_MIN = -5000;
    const int WORLD_HEIGHT_MAX = 5000;
    const int WORLD_DEPTH_MIN = -300;
    const int WORLD_DEPTH_MAX = 300;

    // --- Ventana & Cámara ---
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;
    const int WINDOW_MIN_WIDTH = 1024;
    const int WINDOW_MIN_HEIGHT = 576;
    const float CAMERA_INITIAL_ZOOM = 2.0f;

    // --- Simulación & Generación ---
    const int ATOM_TYPES[] = { 1, 6, 7, 8, 15, 16 }; // H, C, N, O, P, S
    const int ATOM_TYPES_COUNT = 6;

    // --- Física ---
    const float DAMPING = 0.985f;    // Fricción constante
    const float REBOTE = -0.8f;     // Pérdida de energía al chocar
    const float THERMODYNAMIC_JITTER = 0.5f; // Vibración base natural (Browniana)
    const float GRID_CELL_SIZE = 100.0f;     // Tamaño de celda para SpatialGrid
    
    // --- Interacción (Tractor Beam) ---
    const float TRACTOR_FORCE = 2.0f;
    const float TRACTOR_ATTENUATION = 0.01f;
    const float TRACTOR_MAX_SPEED = 4.0f;
    const float TRACTOR_STEER_FACTOR = 0.1f;
    const float TRACTOR_DAMPING = 0.85f;
    const float TRACTOR_PICKUP_RANGE = 50.0f;
    const float TRACTOR_JITTER_INTENSITY = 8.0f; // Fuerza de vibración térmica
    const float TRACTOR_DOCKING_RANGE = 40.0f;   // Distancia para auto-ensamblaje
    const float TRACTOR_DOCKING_FORCE = 0.5f;    // Suavizado hacia el slot
    const float TRACTOR_REACH_MIN = 10.0f;       // Distancia mínima de captura
    const float TRACTOR_HOLD_DAMPING = 0.5f;     // Frenado al capturar
    const float TRACTOR_JITTER_GRADIENT = 100.0f; // Escala de caída del jitter

    // --- UI Labels ---
    const float LABEL_ATOM_THRESHOLD = 0.45f;   
    const int LABEL_FONT_SIZE = 9;
    const float LABEL_FADE_SPEED = 3.0f;
    
    // --- Player Config ---
    inline constexpr float PLAYER_VISUAL_SCALE = 1.6f;
    inline constexpr float PLAYER_SPEED = 250.0f;
    inline constexpr float PLAYER_ACCEL = 0.15f;
    inline constexpr float PLAYER_FRICTION = 0.92f;
    inline constexpr int PLAYER_SYMBOL_FONT = 14;
    inline constexpr int PLAYER_SYMBOL_OFFSET_X = -4;
    inline constexpr int PLAYER_SYMBOL_OFFSET_Y = -6;
    
    // --- HUD SCANNER ---
    const int HUD_HEIGHT = 70;
    const int HUD_FONT_TITLE = 14;
    const int HUD_FONT_INFO = 10;
    const int HUD_FONT_ZOOM = 12;

    // --- UI INSPECTOR ---
    const int INSPECTOR_WIDTH = 170;
    const int INSPECTOR_HEIGHT = 280;
    const int INSPECTOR_MARGIN = 15;
    const int INSPECTOR_FONT_HEADER = 12;
    const int INSPECTOR_FONT_BODY = 10;
    const int INSPECTOR_FONT_SYMBOL = 20;

    // --- UI MOLECULE VIEW (Placeholder) ---
    inline constexpr int MOL_VIEW_X = 15;
    inline constexpr int MOL_VIEW_WIDTH = 170;
    inline constexpr int MOL_VIEW_HEIGHT = 100;
    inline constexpr int MOL_VIEW_Y_OFFSET = 150;
    inline constexpr int INITIAL_ATOM_COUNT = 400;
    inline constexpr float INITIAL_VEL_RANGE = 2.0f; // +- 2.0f
    
    // --- THEME & AESTHETICS ---
    inline constexpr Color THEME_BACKDROP = (Color){ 10, 10, 15, 255 }; // 0x0a0a0fff
    inline constexpr Color THEME_BORDER = (Color){ 40, 44, 52, 255 };
    inline constexpr Color THEME_ACCENT   = GOLD;
    inline constexpr Color THEME_TEXT_PRIMARY = WHITE;
    inline constexpr Color THEME_TEXT_SECONDARY = GRAY;
    inline constexpr Color THEME_HIGHLIGHT = SKYBLUE;
    inline constexpr Color THEME_WARNING = ORANGE;
    inline constexpr Color THEME_SUCCESS = LIME;
    
    inline constexpr float THEME_ROUNDNESS = 0.05f;
    inline constexpr int THEME_BORDER_WIDTH = 1;
    inline constexpr float THEME_HEADER_OPACITY = 0.3f;
    inline constexpr float THEME_BG_OPACITY = 0.95f;

    // --- Visual Core ---
    inline constexpr float DEPTH_SCALE_FACTOR = 0.004f; // Perspectiva suave
    inline constexpr float BASE_ATOM_RADIUS = 6.0f; // Tamaño premium
    inline constexpr float TRACTOR_BEAM_WIDTH = 2.0f;
    inline constexpr float TRACTOR_TARGET_CIRCLE = 25.0f;
    inline constexpr int COLOR_BRIGHTNESS_BASE = 200;
    inline constexpr int MIN_BRIGHTNESS = 50;
}

#endif
