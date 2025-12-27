#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "raylib.h"

namespace Config {
    // --- PHYSICS CONSTANTS ---
    // --- PHYSICS CONSTANTS ---
    inline constexpr float DRAG_COEFFICIENT = 0.95f;    // Reduced from 0.99 to allow momentum
    inline constexpr float WORLD_BOUNCE = -0.5f;
    inline constexpr float BOND_COMPRESSION = 1.2f; 
    inline constexpr float BOND_SNAP_THRESHOLD = 0.45f;
    inline constexpr float BOND_AUTO_RANGE = 55.0f; // Increased for better "grab" (was 50.0)
    inline constexpr int BONDING_THROTTLE_FRAMES = 6;  // Execute every 6 frames (10 Hz) 
    inline constexpr float THERMODYNAMIC_JITTER = 2.5f; // Increased from 0.5 to promote bending/mixing
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
    inline constexpr float SPAWN_RANGE_XY = 1200.0f; // Concentrated spawn (was 1500.0f)
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
    inline constexpr int INITIAL_ATOM_COUNT = 2500; // Increased density (was 1000)
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
    inline constexpr float COULOMB_CONSTANT = 2000.0f;    // Increased slightly 
    inline constexpr float CHARGE_DAMPING = 0.90f;      // Damping for electric forces
    inline constexpr float MIN_COULOMB_DIST = 38.0f;    // Matches BOND_IDEAL_DIST (approx) to prevent compression
    inline constexpr float EM_REACH = 150.0f;           // Max range for electric influence
    inline constexpr float POLARITY_FACTOR = 0.15f;     // Electronegativity -> Partial charge factor
    
    // --- ELASTICITY & RUPTURE (Dynamic Geometry) ---
    inline constexpr float BOND_SPRING_K = 8.0f;        // Softer springs (was 15.0) to allow ring bending
    inline constexpr float BOND_DAMPING = 0.92f;        // Higher damping (was 0.85) to stabilize soft springs
    inline constexpr float BOND_BREAK_STRESS = 180.0f;  // Even stronger to handle larger scale (was 150.0)
    inline constexpr float BOND_IDEAL_DIST = 42.0f;     // Massive increase to ensure VISIBLE GAP (was 24.0)
    inline constexpr float MAX_BOND_RENDER_DIST = 80.0f; // Scale render dist too

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
    inline constexpr float PLAYER_VISUAL_SCALE = 1.8f; // Increased visibility (was 1.6f)
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
    inline constexpr float BASE_ATOM_RADIUS = 7.0f; // Larger atoms (was 6.0f)
    inline constexpr int COLOR_BRIGHTNESS_BASE = 200;
    inline constexpr int MIN_BRIGHTNESS = 50;
    inline constexpr float RENDER_BOND_THICKNESS_BG = 4.0f;
    inline constexpr float RENDER_BOND_THICKNESS_FG = 2.5f;
    inline constexpr float RENDER_MIN_SCALE = 0.1f;

    // --- DEBUG: STRUCTURE FORMATION ---
    inline constexpr bool DEBUG_INSTANT_FORMATION = true;
    inline constexpr bool DEBUG_STRUCTURE_LOGS = true;
    inline constexpr bool DEBUG_DISABLE_STRUCTURE_DAMPING = false;

    // --- PHASE 30: ARCHITECTURAL STANDARDIZATION ---
    namespace Physics {
        inline constexpr float FORMATION_PULL_MULTIPLIER = 80.0f;
        inline constexpr float CARBON_AFFINITY_MIN_DIST = 30.0f;
        inline constexpr float CARBON_AFFINITY_MAX_DIST = 150.0f;
        inline constexpr float CARBON_AFFINITY_STRENGTH_EXTERNAL = 15.0f;
        inline constexpr float CARBON_AFFINITY_STRENGTH_INTERNAL = 10.0f;
        
        inline constexpr float RING_FOLDING_MIN_DIST = 20.0f;
        inline constexpr float RING_FOLDING_MAX_DIST = 300.0f;
        inline constexpr float RING_FOLDING_STRENGTH = 18.0f;

        inline constexpr float Z_FLATTEN_STRENGTH = 20.0f;
        inline constexpr float Z_DAMPING = 0.5f;
        inline constexpr float RING_SPRING_MULTIPLIER = 2.0f;

        inline constexpr float DRIFT_DAMPING_FALLBACK = 0.2f;
    }
}

#endif // CONFIG_HPP
