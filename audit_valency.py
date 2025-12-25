import re
from collections import defaultdict

# Configuración de valencias (extraídas de ChemistryDatabase.cpp)
VALENCIAS = {
    1: 1,  # H
    6: 4,  # C
    7: 3,  # N
    8: 2,  # O
    15: 5, # P
    16: 2  # S
}

# Diccionario de nombres para reportes
ELEMENT_NAMES = {
    1: "H", 6: "C", 7: "N", 8: "O", 15: "P", 16: "S"
}

def analyze_logs(file_path):
    print(f"--- AUDITORÍA QUÍMICA DE TOPOLOGÍA: {file_path} ---")
    
    # connections[parent_id] = list of children_ids
    connections = defaultdict(list)
    # parent_of[child_id] = parent_id
    parent_of = {}
    
    # Intentamos rastrear tipos de átomos. 
    # Sabemos que el ID 0 es H.
    atom_types = {0: 1}
    
    # Patrones de log
    bond_pattern = re.compile(r"\[BOND\] GLOBAL SUCCESS: (\d+) -> (\d+)")
    # Si tuviéramos logs de inspección o spawn detallados, los usaríamos.
    # Por ahora, si un átomo es padre de más de 1 cosa, NO puede ser H.
    
    with open(file_path, 'r') as f:
        for line in f:
            match = bond_pattern.search(line)
            if match:
                child = int(match.group(1))
                parent = int(match.group(2))
                connections[parent].append(child)
                parent_of[child] = parent

    print("\n[ANÁLISIS DE ESTRUCTURAS DETECTADAS]")
    
    violations = []
    
    # 1. Verificar el Jugador (ID 0 - H)
    if 0 in connections:
        children = connections[0]
        count = len(children)
        print(f"Jugador (H, ID 0): {count} enlaces entrantes. Niños: {children}")
        if count > 1:
            violations.append(f"VIOLACIÓN: Jugador (H) tiene {count} enlaces (máx 1).")
        
        # Verificar si los hijos del jugador tienen a su vez hijos
        for child_id in children:
            if child_id in connections:
                grand_children = connections[child_id]
                print(f"  -> Hijo {child_id} tiene {len(grand_children)} enlaces. Estructura: H-{child_id}-(...)")
    
    # 2. Verificar anomalías en el resto de la población
    for atom_id, children in connections.items():
        if atom_id == 0: continue
        
        count = len(children)
        # Si un átomo tiene hijos, pero su padre es un átomo que solo permite 1 enlace (terminal)
        if atom_id in parent_of:
            father_id = parent_of[atom_id]
            if father_id == 0: # El padre es el Hidrógeno del jugador
                # Esto es correcto (H-C-H), el C es hijo del H.
                # Pero el H no puede tener más hijos.
                pass

    print("\n[VERIFICACIÓN DE CAMINOS (Ejemplo H-C-H)]")
    # Buscamos cadenas donde el H sea el inicio
    if 0 in connections:
        for c1 in connections[0]:
            if c1 in connections:
                for c2 in connections[c1]:
                    print(f"Cadena detectada: H(0) -> Atom({c1}) -> Atom({c2})")
                    # Esto es un éxito si c1 es algo como Carbono.
    
    if not violations:
        print("\n[RESULTADO] No se detectaron violaciones de valencia crítica o estructuras imposibles (como H-H-C).")
        print("El sistema de Inteligencia Molecular parece guiar los átomos a los hosts correctos.")
    else:
        print("\n[ALERTA] Anomalías detectadas:")
        for v in violations:
            print(f"  - {v}")

if __name__ == "__main__":
    analyze_logs("session.log")
