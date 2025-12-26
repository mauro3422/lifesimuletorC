# 1. Rutas (Corregidas segun la estructura real de raylib extraido)
$BASE_DIR = $PSScriptRoot
Set-Location $BASE_DIR

$RAYLIB_DIR = "$BASE_DIR/external/raylib/raylib-5.0_win64_mingw-w64"
$INCLUDE_DIR = "$RAYLIB_DIR/include"
$LIB_DIR = "$RAYLIB_DIR/lib"

# 2. Compilacion
g++ src/main.cpp `
    src/core/LocalizationManager.cpp `
    src/core/JsonLoader.cpp `
    src/physics/PhysicsEngine.cpp `
    src/physics/SpatialGrid.cpp `
    src/physics/BondingSystem.cpp `
    src/rendering/Renderer25D.cpp `
    src/input/InputHandler.cpp `
    src/chemistry/ChemistryDatabase.cpp `
    src/gameplay/Player.cpp `
    src/gameplay/TractorBeam.cpp `
    src/ui/LabelSystem.cpp `
    src/ui/Inspector.cpp `
    src/ui/HUD.cpp `
    src/ui/UIWidgets.cpp `
    src/ui/Quimidex.cpp `
    src/gameplay/MissionManager.cpp `
    -I"$INCLUDE_DIR" `
    -I"$BASE_DIR/src" `
    -L"$LIB_DIR" `
    -lraylib -lopengl32 -lgdi32 -lwinmm `
    -static-libgcc -static-libstdc++ `
    -O2 -Wall -std=c++17 `
    -o LifeSimulator.exe

# 3. Ejecucion
if ($?) {
    Write-Host "--- Build Completado con EXITO ---" -ForegroundColor Green
    ./LifeSimulator.exe
}
else {
    Write-Host "Error en la compilacion." -ForegroundColor Red
}
