#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "raylib.h"

namespace Config {
    // --- PHYSICS CONSTANTS ---
    inline constexpr float DRAG_COEFFICIENT = 0.99f;
    inline constexpr float WORLD_BOUNCE = -0.5f;
    inline constexpr float BOND_COMPRESSION = 1.6f; 
    inline constexpr float BOND_SNAP_THRESHOLD = 0.45f;
    inline constexpr float BOND_AUTO_RANGE = 30.0f;
    inline constexpr int BONDING_THROTTLE_FRAMES = 6;  // Execute every 6 frames (10 Hz) 
    inline constexpr float THERMODYNAMIC_JITTER = 0.5f; 
    inline constexpr float GRID_CELL_SIZE = 100.0f;     
    inline constexpr float PHYSICS_EPSILON = 0.001f;
    inline constexpr float FLOAT_MAX = 1.0e30f;
    inline constexpr float CHARGE_THRESHOLD = 0.001f; // Threshold for electromagnetic influence
    inline constexpr float SPAWN_VEL_DIVISOR = 100.0f;

    
    // --- WORLD DIMENSIONS & SPAWN ---
    inline constexpr int WORLD_WIDTH_MIN = -5000;
    inline constexpr int WORLD_WIDTH_MAX = 5000;
    inline constexpr int WORLD_HEIGHT_MIN = -5000;
    inline constexpr int WORLD_HEIGHT_MAX = 5000;
    inline constexpr float SPAWN_RANGE_XY = 250.0f;
    inline constexpr float SPAWN_RANGE_Z = 40.0f;
    inline constexpr float INITIAL_VEL_RANGE = 2.0f;
    inline constexpr int WORLD_DEPTH_MIN = -300;
    inline constexpr int WORLD_DEPTH_MAX = 300;

    // --- WINDOW & CAMERA ---
    inline constexpr int WINDOW_WIDTH = 1280;
    inline constexpr int WINDOW_HEIGHT = 720;
    inline constexpr int WINDOW_MIN_WIDTH = 1024;
    inline constexpr int WINDOW_MIN_HEIGHT = 576;
    inline constexpr float CAMERA_INITIAL_ZOOM = 2.0f;
    inline constexpr float CAMERA_FOLLOW_SPEED = 6.0f;
    inline constexpr float CAMERA_ZOOM_SMOOTH = 2.5f;
    inline constexpr float CAMERA_ZOOM_MIN = 0.05f;
    inline constexpr float CAMERA_ZOOM_MAX = 15.0f;
    inline constexpr float CAMERA_ZOOM_WHEEL_SENSITIVITY = 0.15f;

    // --- SIMULATION ---
    inline constexpr int INITIAL_ATOM_COUNT = 1000;
    inline constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;
    inline constexpr float MAX_FRAME_TIME = 0.25f;
    
    // --- INTERACTION ---
    inline constexpr float TRACTOR_FORCE = 5.0f; // Initial pull force
    inline constexpr float TRACTOR_ATTENUATION = 0.01f;
    inline constexpr float TRACTOR_MAX_SPEED = 500.0f;
    inline constexpr float TRACTOR_STEER_FACTOR = 0.12f; // Smoother steering
    inline constexpr float TRACTOR_DAMPING = 0.97f;
    inline constexpr float TRACTOR_PICKUP_RANGE = 70.0f;
    inline constexpr float TRACTOR_JITTER_INTENSITY = 8.0f; 
    inline constexpr float TRACTOR_DOCKING_RANGE = 40.0f;   
    inline constexpr float TRACTOR_DOCKING_FORCE = 0.5f;    
    inline constexpr float TRACTOR_REACH_MIN = 25.0f;       
    inline constexpr float TRACTOR_HOLD_DAMPING = 0.1f;     
    inline constexpr float TRACTOR_JITTER_GRADIENT = 100.0f; 
    inline constexpr float TRACTOR_BEAM_WIDTH = 2.0f;
    inline constexpr float TRACTOR_TARGET_CIRCLE = 25.0f;
    
    // --- CHEMISTRY & ELECTROMAGNETISM ---
    inline constexpr float COULOMB_CONSTANT = 1800.0f;    // Attract/Repel force coefficient
    inline constexpr float CHARGE_DAMPING = 0.90f;      // Damping for electric forces
    inline constexpr float MIN_COULOMB_DIST = 18.0f;    // Avoid singularities (soft-core repulsion)
    inline constexpr float EM_REACH = 150.0f;           // Max range for electric influence
    inline constexpr float POLARITY_FACTOR = 0.15f;     // Electronegativity -> Partial charge factor
    
    // --- ELASTICITY & RUPTURE (Dynamic Geometry) ---
    inline constexpr float BOND_SPRING_K = 15.0f;       // Bond "spring" constant
    inline constexpr float BOND_DAMPING = 0.85f;        // Vibration damping
    inline constexpr float BOND_BREAK_STRESS = 35.0f;   // Break distance (max stress)
    inline constexpr float BOND_IDEAL_DIST = 18.0f;     // Ideal bond length (rest position)
    inline constexpr float MAX_BOND_RENDER_DIST = 40.0f; // Max distance to render bond lines (prevents visual glitches)

    // --- BONDING ANIMATION ---
    inline constexpr float BOND_DOCKING_SPEED = 0.04f; // Slower speed for smoother docking
    inline constexpr float BOND_LERP_POS = 0.2f;
    inline constexpr float BOND_LERP_VEL = 0.3f;

    // --- INPUT ---
    inline constexpr float INPUT_DOUBLE_SPACE_THRESHOLD = 0.3f;
    
    // --- UI LABELS ---
    inline constexpr float LABEL_ATOM_THRESHOLD = 0.45f;   
    inline constexpr int LABEL_FONT_SIZE = 9;
    inline constexpr float LABEL_FADE_SPEED = 3.0f;
    
    // --- PLAYER ---
    inline constexpr float PLAYER_VISUAL_SCALE = 1.6f;
    inline constexpr float PLAYER_SPEED = 35.0f;
    inline constexpr float PLAYER_ACCEL = 0.05f;
    inline constexpr int PLAYER_SYMBOL_FONT = 14;
    inline constexpr int PLAYER_SYMBOL_OFFSET_X = -4;
    inline constexpr int PLAYER_SYMBOL_OFFSET_Y = -6;
    
    // --- HUD SCANNER ---
    inline constexpr int HUD_HEIGHT = 70;
    inline constexpr int HUD_FONT_TITLE = 14;
    inline constexpr int HUD_FONT_INFO = 10;
    inline constexpr int HUD_FONT_ZOOM = 12;

    // --- UI INSPECTOR ---
    inline constexpr int INSPECTOR_WIDTH = 190;
    inline constexpr int INSPECTOR_HEIGHT = 280;
    inline constexpr int INSPECTOR_MARGIN = 15;
    inline constexpr int INSPECTOR_FONT_HEADER = 12;
    inline constexpr int INSPECTOR_FONT_BODY = 10;
    inline constexpr int INSPECTOR_FONT_SYMBOL = 20;

    // --- UI MOLECULE VIEW ---
    inline constexpr int MOL_VIEW_X = 15;
    inline constexpr int MOL_VIEW_WIDTH = 170;
    inline constexpr int MOL_VIEW_HEIGHT = 100;
    inline constexpr int MOL_VIEW_Y_OFFSET = 150;
    
    // --- THEME & AESTHETICS ---
    inline constexpr Color THEME_BACKDROP = { 10, 10, 15, 255 }; 
    inline constexpr Color THEME_BORDER = { 40, 44, 52, 255 };
    inline constexpr Color THEME_ACCENT   = GOLD;
    inline constexpr Color THEME_TEXT_PRIMARY = WHITE;
    inline constexpr Color THEME_TEXT_SECONDARY = GRAY;
    inline constexpr Color THEME_HIGHLIGHT = SKYBLUE;
    inline constexpr Color THEME_WARNING = ORANGE;
    inline constexpr Color THEME_SUCCESS = LIME;
    inline constexpr Color THEME_INFO = SKYBLUE;
    inline constexpr Color THEME_DANGER = RED;
    
    inline constexpr float THEME_ROUNDNESS = 0.05f;
    inline constexpr int THEME_BORDER_WIDTH = 1;
    inline constexpr float THEME_HEADER_OPACITY = 0.3f;
    inline constexpr float THEME_BG_OPACITY = 0.95f;

    // --- VISUAL CORE ---
    inline constexpr float DEPTH_SCALE_FACTOR = 0.004f; 
    inline constexpr float BASE_ATOM_RADIUS = 6.0f; 
    inline constexpr int COLOR_BRIGHTNESS_BASE = 200;
    inline constexpr int MIN_BRIGHTNESS = 50;
    inline constexpr float RENDER_BOND_THICKNESS_BG = 4.0f;
    inline constexpr float RENDER_BOND_THICKNESS_FG = 2.5f;
    inline constexpr float RENDER_MIN_SCALE = 0.1f;
}

#endif // CONFIG_HPP
