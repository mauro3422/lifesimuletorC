$BASE_DIR = $PSScriptRoot
Set-Location $BASE_DIR

$RAYLIB_DIR = "$BASE_DIR/external/raylib/raylib-5.0_win64_mingw-w64"
$INCLUDE_DIR = "$RAYLIB_DIR/include"
$LIB_DIR = "$RAYLIB_DIR/lib"

Write-Host "Compiling EDGE CASE TESTS..." -ForegroundColor Cyan

# Explicitly list source files
$sources = @(
    "src/tests/test_edge_cases.cpp",
    "src/core/LocalizationManager.cpp",
    "src/core/JsonLoader.cpp",
    "src/physics/BondingSystem.cpp",
    "src/physics/PhysicsEngine.cpp",
    "src/physics/SpatialGrid.cpp",
    "src/physics/StructuralPhysics.cpp",
    "src/chemistry/ChemistryDatabase.cpp",
    "src/chemistry/StructureRegistry.cpp",
    "src/gameplay/MissionManager.cpp"
)

# Run g++ with explicit quoting for paths
g++ $sources `
    -I"$INCLUDE_DIR" `
    -I"$BASE_DIR/src" `
    -I"$BASE_DIR/src/core" `
    -I"$BASE_DIR/src/physics" `
    -I"$BASE_DIR/src/ecs" `
    -I"$BASE_DIR/src/chemistry" `
    -I"$BASE_DIR/src/gameplay" `
    -L"$LIB_DIR" `
    -lraylib -lopengl32 -lgdi32 -lwinmm `
    -std=c++17 `
    -o test_edge_cases.exe

if ($?) {
    Write-Host "Build Successful! Running tests..." -ForegroundColor Green
    ./test_edge_cases.exe
}
else {
    Write-Host "Build Failed!" -ForegroundColor Red
    exit 1
}
