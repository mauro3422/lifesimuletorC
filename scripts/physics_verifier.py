
import math

# --- LATEST C++ CONFIG (Build 6784067) ---
BOND_SPRING_K = 8.0
BOND_DAMPING = 0.92
BOND_BREAK_STRESS = 180.0
BOND_IDEAL_DIST = 42.0 # The fix
DT = 1.0 / 60.0

# --- VISUAL CONFIG (from Config.hpp & elements.json) ---
BASE_ATOM_RADIUS = 7.0
VDW_CARBON = 1.7
PLAYER_SCALE = 1.0 # Assuming standard atoms are 1.0x scale relative to base? 
# In Renderer25D: radius = (element.vdWRadius * Config::BASE_ATOM_RADIUS) * scale;
# Scale depends on Z, but at Z=0 (creation plane), scale is ~1.0 + offset. 
# Let's assume Z=0 for base check.

ATOM_RADIUS_PX = VDW_CARBON * BASE_ATOM_RADIUS # 1.7 * 7.0 = 11.9 px
ATOM_DIAMETER_PX = ATOM_RADIUS_PX * 2 # 23.8 px

MASS = 12.0

class Atom:
    def __init__(self, id, x, y):
        self.id = id
        self.x = float(x)
        self.y = float(y)
        self.vx = 0.0
        self.vy = 0.0
        self.fx = 0.0
        self.fy = 0.0

    def apply_force(self, fx, fy):
        self.fx += fx
        self.fy += fy

    def update(self):
        ax = self.fx / MASS
        ay = self.fy / MASS
        self.vx += ax * DT
        self.vy += ay * DT
        self.vx *= 0.95
        self.vy *= 0.95
        self.x += self.vx * DT
        self.y += self.vy * DT
        self.fx = 0.0
        self.fy = 0.0

def dist(a, b):
    return math.sqrt((a.x - b.x)**2 + (a.y - b.y)**2)

def solve_bond(a, b):
    d = dist(a, b)
    if d == 0: return False
    
    # Physics Spring
    force = (d - BOND_IDEAL_DIST) * BOND_SPRING_K
    
    dx = b.x - a.x
    dy = b.y - a.y
    fx = (dx/d) * force
    fy = (dy/d) * force
    
    # Damping
    rx = b.vx - a.vx
    ry = b.vy - a.vy
    damp = (rx * (dx/d) + ry * (dy/d)) * BOND_DAMPING
    fx += (dx/d) * damp
    fy += (dy/d) * damp

    a.apply_force(fx, fy)
    b.apply_force(-fx, -fy)
    return True

# Setup C4 Square
atoms = [Atom(0,0,0), Atom(1,42,0), Atom(2,42,42), Atom(3,0,42)]
bonds = [(0,1), (1,2), (2,3), (3,0)]

print(f"--- DIAGNOSTICS: Carbon Ring (C4) ---")
print(f"Physics Dist Target: {BOND_IDEAL_DIST} px")
print(f"Atom Radius: {ATOM_RADIUS_PX:.2f} px")
print(f"Atom Diameter: {ATOM_DIAMETER_PX:.2f} px")
print(f"Visual Collision Threshold: {ATOM_DIAMETER_PX:.2f} px")

min_gap = 1000.0

for step in range(300):
    for (i,j) in bonds: solve_bond(atoms[i], atoms[j])
    for a in atoms: a.update()
    
    # Check Gap for Bond 0-1
    d = dist(atoms[0], atoms[1])
    gap = d - ATOM_DIAMETER_PX
    if gap < min_gap: min_gap = gap

print(f"\n--- RESULTS after 300 steps ---")
final_dist = dist(atoms[0], atoms[1])
final_gap = final_dist - ATOM_DIAMETER_PX

print(f"Final Bond Length: {final_dist:.2f} px")
print(f"Final Visual Gap:  {final_gap:.2f} px")

if final_gap > 5.0:
    print("[PASS] VISUALS CLEAR. Atoms are separated by visible bond line.")
elif final_gap > 0:
    print("[WARN] VISUALS TIGHT. Atoms are barely touching.")
else:
    print("[FAIL] OVERLAP DETECTED. Atoms are clipping!")
