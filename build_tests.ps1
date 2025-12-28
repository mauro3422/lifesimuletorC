$BASE_DIR = $PSScriptRoot
Set-Location $BASE_DIR

$RAYLIB_DIR = "$BASE_DIR/external/raylib/raylib-5.0_win64_mingw-w64"
$INCLUDE_DIR = "$RAYLIB_DIR/include"
$LIB_DIR = "$RAYLIB_DIR/lib"

# Common source files needed by all tests
$common_sources = @(
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

# Common compiler flags
$common_flags = @(
    "-I`"$INCLUDE_DIR`"",
    "-I`"$BASE_DIR/src`"",
    "-I`"$BASE_DIR/src/core`"",
    "-I`"$BASE_DIR/src/physics`"",
    "-I`"$BASE_DIR/src/ecs`"",
    "-I`"$BASE_DIR/src/chemistry`"",
    "-I`"$BASE_DIR/src/gameplay`"",
    "-L`"$LIB_DIR`"",
    "-lraylib", "-lopengl32", "-lgdi32", "-lwinmm",
    "-std=c++17", "-pthread"
)

$all_passed = $true

# ============================================================================
# TEST 1: Edge Cases
# ============================================================================
Write-Host "`n=== Compiling EDGE CASE TESTS ===" -ForegroundColor Cyan

$sources = @("src/tests/test_edge_cases.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $($common_flags -join ' ') -o test_edge_cases.exe"
Invoke-Expression $cmd

if ($?) {
    Write-Host "Build Successful! Running..." -ForegroundColor Green
    ./test_edge_cases.exe
    if (-not $?) { $all_passed = $false }
}
else {
    Write-Host "Build Failed!" -ForegroundColor Red
    $all_passed = $false
}

# ============================================================================
# TEST 2: Polymerization
# ============================================================================
Write-Host "`n=== Compiling POLYMERIZATION TESTS ===" -ForegroundColor Cyan

$sources = @("src/tests/test_polymerization.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $($common_flags -join ' ') -o test_polymerization.exe"
Invoke-Expression $cmd

if ($?) {
    Write-Host "Build Successful! Running..." -ForegroundColor Green
    ./test_polymerization.exe
    if (-not $?) { $all_passed = $false }
}
else {
    Write-Host "Build Failed!" -ForegroundColor Red
    $all_passed = $false
}

# ============================================================================
# TEST 3: BondingCore Unit Tests
# ============================================================================
Write-Host "`n=== Compiling BONDING CORE TESTS ===" -ForegroundColor Cyan

$sources = @("src/tests/test_bonding_core.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $($common_flags -join ' ') -o test_bonding_core.exe"
Invoke-Expression $cmd

if ($?) {
    Write-Host "Build Successful! Running..." -ForegroundColor Green
    ./test_bonding_core.exe
    if (-not $?) { $all_passed = $false }
}
else {
    Write-Host "Build Failed!" -ForegroundColor Red
    $all_passed = $false
}

# ============================================================================
# TEST 4: Ring Chemistry Tests
# ============================================================================
Write-Host "`n=== Compiling RING CHEMISTRY TESTS ===" -ForegroundColor Cyan

$sources = @("src/tests/test_ring_chemistry.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $($common_flags -join ' ') -o test_ring_chemistry.exe"
Invoke-Expression $cmd

if ($?) {
    Write-Host "Build Successful! Running..." -ForegroundColor Green
    ./test_ring_chemistry.exe
    if (-not $?) { $all_passed = $false }
}
else {
    Write-Host "Build Failed!" -ForegroundColor Red
    $all_passed = $false
}

# ============================================================================
# TEST 5: Animation System Tests
# ============================================================================
Write-Host "`n=== Compiling ANIMATION TESTS ===" -ForegroundColor Cyan

$sources = @("src/tests/test_animation.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $($common_flags -join ' ') -o test_animation.exe"
Invoke-Expression $cmd

if ($?) {
    Write-Host "Build Successful! Running..." -ForegroundColor Green
    ./test_animation.exe
    if (-not $?) { $all_passed = $false }
}
else {
    Write-Host "Build Failed!" -ForegroundColor Red
    $all_passed = $false
}

# ============================================================================
# FINAL SUMMARY
# ============================================================================
Write-Host "`n============================================" -ForegroundColor White
if ($all_passed) {
    Write-Host "ALL TEST SUITES PASSED!" -ForegroundColor Green
    exit 0
}
else {
    Write-Host "SOME TESTS FAILED - Check output above" -ForegroundColor Red
    exit 1
}
