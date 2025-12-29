# 🧬 Referencia Biológica - LifeSimulator

## Orden Evolutivo Real (Tiers del Juego)

### Tier 0: Química Informativa Prebiótica
- **Cianuros y Nitrilos**: Cadenas de HCN. No es vida, pero es "código máquina". 
- **Mecánica**: Otorga control de precisión sobre los átomos.

### Tier 1: Sistemas Compartimentados (La Protocélula)
- **Lípidos y Membranas**: Cadenas de Carbono. Crean el "interior" vs "exterior".
- **Mecánica**: Reducción de entropía (Efecto Jaula) que permite formar anillos estables.

### Tier 2: El Mundo de ARN
- **A, U, G, C**: Las primeras instrucciones reales.

---

## Escalas Reales

| Estructura | Cantidad Real | Para el Juego |
|------------|--------------|---------------|
| 1 Nucleótido | 15-20 átomos | 15-20 átomos |
| 1 Codón (3 nucleótidos) | ~50 átomos | ~50 átomos |
| Gen mínimo funcional | ~300 nucleótidos | 10-50 nucleótidos |
| ADN Humano | 6 mil millones pares | Automatización |

### Automatización (Tu idea)

El jugador debe crear "fábricas moleculares" que copien automáticamente:
- **Ribosoma** = Lee ARN → produce proteínas
- **Polimerasa** = Copia ADN/ARN
- **ATP Sintasa** = Genera energía

---

## Moléculas para Poderes/Stats

### 🔥 Resistencia al Calor
```
Moléculas necesarias:
├── Proteínas con S (azufre) - Puentes disulfuro
├── Prolina (C5H9NO2) - Rigidez estructural
└── Trehalosa (C12H22O11) - Protector térmico

Efecto: +Resistencia a temperatura
```

### 🧊 Resistencia al Frío
```
Moléculas necesarias:
├── Glicerol (C3H8O3) - Anticongelante natural
├── Proteínas AFP (con Alanina repetida)
└── Lípidos insaturados - Membranas flexibles

Efecto: +Resistencia a congelamiento
```

### ⚡ Más Energía
```
Moléculas necesarias:
├── ATP (C10H16N5O13P3) - Moneda de energía
├── NAD+ (C21H27N7O14P2) - Transportador de electrones
└── Citocromo C (contiene Fe) - Cadena respiratoria

Efecto: +Velocidad de acciones
```

### 🛡️ Resistencia a Radiación
```
Moléculas necesarias:
├── Melanina (polímero de tirosina)
├── Glutatión (C10H17N3O6S) - Antioxidante
├── Superóxido dismutasa (contiene Zn, Cu, Mn)

Efecto: +Resistencia a daño UV/radiación
```

### 🧠 Velocidad de Procesamiento (Lectura ADN)
```
Moléculas necesarias:
├── Más ribosomas (proteína + ARN)
├── Helicasa (enzima que abre ADN)
├── ATP extra para el proceso

Efecto: +Velocidad de copia/lectura
```

---

## Componentes de una Célula Mínima

### 1. Membrana (Compartimento)
```
Fosfolípidos:
├── Cabeza: Fosfato + Glicerol (hidrofílica)
└── Cola: Ácidos grasos (hidrofóbica)

Fórmula ejemplo: C42H82NO8P (Fosfatidilcolina)
```

---

## 🧱 Visión de Escala: Del Ladrillo a la Casa

### La Analogía Fundamental

Imagina que estás construyendo una **casa de ladrillos**:

| Nivel | Biología Real | En el Juego | Rol |
|-------|--------------|-------------|-----|
| **Ladrillo** | Hexágono de Carbono (C6) | RigidBody estable | Unidad básica impenetrable |
| **Muro/Pared** | Membrana lipídica | Cadena de hexágonos | Barrera que encierra |
| **Casa** | Célula completa | Espacio interior | Donde ocurre la vida |

### ¿Siempre mismo tamaño los hexágonos?

**Sí.** En química real:
- Un anillo de benceno/grafeno (C6) **siempre** tiene el mismo tamaño exacto
- La distancia de los enlaces C-C es fija (~1.4 Å)  
- Los "ladrillos" son **idénticos**

**Para crecer**: No estiras hexágonos, **agregas más** al borde (como azulejos en un piso).

### Flujo de Construcción

```
PASO 1: Hexágono Básico
┌──────────────────────────────────────┐
│  6 Carbonos → 1 Hexágono (RigidBody) │
│        C──C                           │
│       /    \                          │
│      C      C                         │
│       \    /                          │
│        C──C                           │
└──────────────────────────────────────┘
              │
              ▼
PASO 2: Fusión de Hexágonos
┌──────────────────────────────────────┐
│  2 Hexágonos comparten 1 lado →      │
│  "Placa" de 2 ladrillos              │
│      ┌───┬───┐                       │
│      │ ⬡ │ ⬡ │                       │
│      └───┴───┘                       │
└──────────────────────────────────────┘
              │
              ▼
PASO 3: Crecimiento de Placa
┌──────────────────────────────────────┐
│  Seguir pegando placas...            │
│      ┌───┬───┬───┬───┐               │
│      │ ⬡ │ ⬡ │ ⬡ │ ⬡ │               │
│      └───┴───┴───┴───┘               │
└──────────────────────────────────────┘
              │
              ▼
PASO 4: Curvatura y Cierre (MEMBRANA)
┌──────────────────────────────────────┐
│  La estructura se curva y cierra     │
│  formando una "burbuja"              │
│                                      │
│       ⬡─⬡─⬡─⬡                        │
│      /       \                       │
│     ⬡   💧💧  ⬡  ← Interior          │
│     ⬡   💧💧  ⬡    (Citoplasma)      │
│      \       /                       │
│       ⬡─⬡─⬡─⬡                        │
│                                      │
│  = PROTOCÉLULA                       │
└──────────────────────────────────────┘
```

### El Interior: Caldo de Cultivo

Una vez cerrada la membrana:

- **Adentro** hay un espacio **gigante** (en comparación con 1 átomo)
- Ahí viven las moléculas de agua ($H_2O$), futuras cadenas de ARN, proteínas, etc.
- Las moléculas **rebotan** contra las paredes de hexágonos pero **no se escapan**
- Es una "piscina molecular" protegida

### Física Requerida

| Sistema | Estado | Descripción |
|---------|--------|-------------|
| Hexago Formation | ✅ Existe | 6C → anillo rígido |
| Cycle Bonds | ✅ Existe | Enlaces no-jerárquicos |
| Hexago Fusion | 🚧 Futuro | Pegar hexágonos por lado compartido |
| Boundary Detection | 🚧 Futuro | Detectar "adentro" vs "afuera" |
| Membrane Impermeability | 🚧 Futuro | Colisiones contra muro de hexágonos |

---

### 2. Sistema de Energía
```
ATP Sintasa:
├── Parte F0: Canal de protones
└── Parte F1: Sintetiza ATP

ADP + Pi + Energía → ATP
```

### 3. Sistema de Información
```
ADN → Transcripción → ARN → Traducción → Proteína
        (ARN Polimerasa)      (Ribosoma)
```

### 4. Sistema de Copia
```
ADN Polimerasa:
├── Lee la cadena original
├── Copia nucleótido por nucleótido
└── Produce réplica exacta
```

---

## Progresión del Jugador

### Fase 1: Química Básica
```
Meta: Formar aminoácidos prebióticos
Logros: 
├── Glicina (C2H5N1O2) - El más simple, bloque de construcción
├── Alanina (C3H7N1O2) - Clave en hélices alfa
├── Serina (C3H7N1O3) - Puente para fosfolípidos
├── Cisteína (C3H7N1O2S1) - Puentes disulfuro (resistencia)
└── Prolina (C5H9N1O2) - Rigidez estructural
```

### Fase 2: Nucleobases (Código Genético)
```
Meta: Formar las bases A, U, G, C, T
Mecánica: Requiere Catálisis en Arcilla o Ventilas
Logros:
├── Adenina (C5H5N5) - Energía (ATP) e Información
├── Guanina (C5H5N5O1) - Estabilidad G-C
├── Citosina (C4H5N3O1) - Código genético
├── Uracilo (C4H4N2O2) - Mundo de ARN
└── Timina (C5H6N2O2) - Estabilidad ADN (Era 7)
```

### Fase 3: Azúcares Primordiales
```
Meta: Estructuras de soporte y energía rápida
Logros:
├── Ribosa (C5H10O5) - El azúcar del ARN
├── Desoxirribosa (C5H10O4) - El azúcar del ADN
├── Glucosa (C6H12O6) - Combustible celular
└── Fructosa (C6H12O6) - Isómero energético
```

### Fase 4: Metabolismo y Radicales
```
Meta: Gestión de energía y defensa
Logros:
├── Ácido Pirúvico (C3H4O3) - Hub metabólico
├── Ácido Láctico (C3H6O3) - Fermentación
├── Radical Hidroxilo (H1O1) - Daño oxidativo / Limpieza
└── Radical Metilo (C1H3) - Epigenética (Control de genes)
```

---

## 🌌 Química Emergente (Sobrevivientes)

La simulación genera moléculas que no existen en la Tierra moderna pero son químicamente posibles. 
Estas moléculas se catalogan como **Exóticas** o **Emergentes**:
- **Radicales Reactivos**: Moléculas de vida corta (ns) que median reacciones complejas.
- **Dímeros Elementales**: S2 (Azufre azul), P2 (Fósforo reactivo) - comunes en ambientes extremos.
- **Compuestos CHNOPS Híbridos**: Combinaciones únicas filtradas por el flujo de **Auditoría Científica**.

---

## Tabla de Átomos → Función

| Átomo | Rol Biológico | Para el Juego |
|-------|--------------|---------------|
| C | Esqueleto de toda la vida | Base de todo |
| H | Enlaces, energía | Abundante |
| O | Respiración, agua | Energía |
| N | ADN, proteínas | Información |
| P | ADN, ATP, membranas | Energía + Datos |
| S | Puentes disulfuro | Resistencia |
| Fe | Hemoglobina, enzimas | Transporte O2 |
| Mg | Clorofila | Fotosíntesis |
| Ca | Estructura | Huesos/Concha |
| Zn | Enzimas | Catálisis |
| Cu | Enzimas | Catálisis |

---

## Mecánica de Catálisis por Arcilla

### Cómo Funciona en la Vida Real
1. **Superficie de concentración**: Moléculas se adhieren a la arcilla
2. **Orientación**: La arcilla orienta las moléculas favorablemente
3. **Estabilización**: Protege intermediarios frágiles
4. **Liberación**: El producto formado se libera

### Implementación en el Juego

#### Opción A: Catálisis Realista (ACTUAL)
- En zonas de arcilla, aumenta la fuerza de atracción C-N
- Mayor probabilidad de formar enlaces cíclicos (anillos)
- Las moléculas mantienen todos sus átomos individuales
- Física realista: el jugador observa la formación gradual

#### Opción C: Compresión de Moléculas (FUTURO)
- Moléculas muy grandes (10+ átomos) se "comprimen" en 1 partícula
- Reduce carga de partículas para mejor rendimiento
- La partícula comprimida tiene stats basados en su composición
- Permite escalar a estructuras más complejas (proteínas, ADN)

### Flujo de Formación de Bases Nitrogenadas
```
Zona Arcilla
    │
    ▼
[Fragmentos C, N, H, O] ──atracción aumentada──▶ [Ciclo C-N]
    │                                                │
    ▼                                                ▼
Si ciclo estable (5-6 miembros) ──────────▶ Base Nitrogenada
    │                                       (A, G, C, T, U)
    ▼
Liberación al medio
```

---

*Actualizado: 2025-12-23*

