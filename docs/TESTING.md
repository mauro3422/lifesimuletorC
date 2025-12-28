# 🧪 Testing Guide

## Running All Tests

```powershell
./run_tests.ps1
```

This unified script compiles and runs all test suites.

---

## Test Suites

### 1. Integration Tests (Ring Topology)

**File**: `tests/test_molecular_geometry.cpp`

Tests the core ring formation and VSEPR geometry:

| Test Category | Assertions |
|---------------|------------|
| VSEPR Angles | 50+ |
| Ring Detection | 7 |
| Ladder Diagnostics | Pass/Fail |

**What it validates:**
- Carbon tetrahedral geometry (109.5°)
- Oxygen bent geometry (104.5°)
- Nitrogen pyramidal geometry (107°)
- 4-atom ring formation and stability

### 2. Standalone Unit Tests

Located in the integration test file with custom macros to avoid Raylib conflicts.

**Categories:**
- `BondingCore` - Slot management, valency checks
- `RingChemistry` - Cycle detection, LCA calculation
- `Animation` - Docking progress, isLocked state

---

## Known Issues

### Raylib + Doctest Conflict

**Problem**: Windows.h macros (`far`, `near`, `Rectangle`) collide with Raylib headers.

**Current Solution**: Integration tests use custom assertion macros:
```cpp
#define TEST_ASSERT(cond) if(!(cond)) { /* custom handler */ }
```

**Future Solution**: Isolate tests that don't need Raylib into pure Doctest files.

---

## Manual Testing Checklist

### Bonding Mechanics
- [ ] Navigate to Clay Island (-1200, -400)
- [ ] Drop 4 Carbon atoms nearby
- [ ] Verify automatic chain formation
- [ ] Wait for cycle closure → square formation
- [ ] Test disconnection → all ring flags cleared

### Tractor Beam
- [ ] Left-click captures single atom
- [ ] Captured atom follows cursor
- [ ] Right-click releases/undoes bonds
- [ ] Valencia Shield prevents garbage accumulation

### UI
- [ ] F1 toggles language (ES ↔ EN)
- [ ] Space opens Inspector
- [ ] Double-Space opens Molecule View
- [ ] Q opens Quimidex

---

## Adding New Tests

1. Add test cases to `tests/test_molecular_geometry.cpp`
2. Use `TEST_ASSERT()` macro for compatibility
3. Run `./run_tests.ps1` to verify
4. Update this doc if adding new test categories

---

*See also: [BUILDING.md](BUILDING.md) for compilation instructions*
