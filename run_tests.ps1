# LifeSimulator Unified Test Runner
# Runs all working tests: real integration tests + standalone geometry

$BASE_DIR = $PSScriptRoot
Set-Location $BASE_DIR

$RAYLIB_DIR = "$BASE_DIR/external/raylib/raylib-5.0_win64_mingw-w64"
$all_passed = $true

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "     LIFESIMULATOR TEST SUITE              " -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# Common sources for integration tests
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

$flags = "-I`"$RAYLIB_DIR/include`" -I`"src`" -L`"$RAYLIB_DIR/lib`" -lraylib -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++ -std=c++17"

# ============================================
# TEST 1: Ring Topology (Integration)
# ============================================
Write-Host "`n>>> Ring Topology Tests <<<" -ForegroundColor Yellow
$sources = @("src/tests/test_ring_topology.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $flags -o test_ring.exe"
Invoke-Expression $cmd 2>$null

if ($?) {
    $output = ./test_ring.exe 2>&1
    $output | Where-Object { $_ -match "TEST|PASS|FAIL|RESULTS" } | ForEach-Object { Write-Host $_ }
    if ($output -match "Failed: 0") { 
        Write-Host "[PASS] Ring Topology" -ForegroundColor Green
    }
    else { 
        Write-Host "[FAIL] Ring Topology" -ForegroundColor Red
        $all_passed = $false 
    }
    Remove-Item test_ring.exe -Force
}
else {
    Write-Host "[BUILD ERROR] Ring Topology" -ForegroundColor Magenta
    $all_passed = $false
}

# ============================================
# TEST 2: Ladder Diagnostics (Integration)
# ============================================
Write-Host "`n>>> Ladder Diagnostics <<<" -ForegroundColor Yellow
$sources = @("src/tests/test_ladder_diag.cpp") + $common_sources
$cmd = "g++ $($sources -join ' ') $flags -o test_ladder.exe"
Invoke-Expression $cmd 2>$null

if ($?) {
    $output = ./test_ladder.exe 2>&1
    $output | Where-Object { $_ -match "TEST|PASS|FAIL|Result" } | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -eq 0) { 
        Write-Host "[PASS] Ladder Diagnostics" -ForegroundColor Green
    }
    else { 
        Write-Host "[FAIL] Ladder Diagnostics" -ForegroundColor Red
        $all_passed = $false 
    }
    Remove-Item test_ladder.exe -Force
}
else {
    Write-Host "[BUILD ERROR] Ladder Diagnostics" -ForegroundColor Magenta
    $all_passed = $false
}

# ============================================
# TEST 3: VSEPR Geometry (Standalone)
# ============================================
Write-Host "`n>>> VSEPR Geometry Tests <<<" -ForegroundColor Yellow
g++ tests/test_molecular_geometry.cpp -I"tests" -std=c++17 -o test_geom.exe 2>$null

if ($?) {
    $output = ./test_geom.exe 2>&1
    if ($output -match "Status: SUCCESS") { 
        $assertions = ($output | Select-String "assertions: (\d+)").Matches.Groups[1].Value
        Write-Host "[PASS] VSEPR Geometry ($assertions assertions)" -ForegroundColor Green
    }
    else { 
        Write-Host "[FAIL] VSEPR Geometry" -ForegroundColor Red
        $all_passed = $false 
    }
    Remove-Item test_geom.exe -Force
}
else {
    Write-Host "[BUILD ERROR] VSEPR Geometry" -ForegroundColor Magenta
    $all_passed = $false
}

# ============================================
# SUMMARY
# ============================================
Write-Host "`n============================================" -ForegroundColor Cyan
if ($all_passed) {
    Write-Host "ALL TESTS PASSED!" -ForegroundColor Green
    exit 0
}
else {
    Write-Host "SOME TESTS FAILED" -ForegroundColor Red
    exit 1
}
