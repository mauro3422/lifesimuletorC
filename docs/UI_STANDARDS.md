# 🎨 Estándares de UI: Nano-HD & Lore-Core

## 📐 Principios de Diseño
- **Escala Compacta**: Fuentes de 9px-12px.
- **Transparencia**: Fondos `THEME_BACKDROP` (Alpha 240).
- **Acentos**: `SKYBLUE` (Tecnología) y `GOLD` (Jugador).
- **Bordes**: Redondeo `THEME_ROUNDNESS = 0.05f`.

## 🖱️ Sistema de Entrada (Input Capture)
Para evitar "dramas" donde un click en la UI también activa algo en el mundo (selección de átomos o rayo tractor), hemos centralizado la detección:

1.  **Captura Automática**: Los widgets (`UIWidgets`) llaman a `input.setMouseCaptured(true)` si el mouse está sobre ellos.
2.  **Protocolo de Mundo**: Toda lógica de selección o interacción con el escenario dentro del `main.cpp` **debe** usar:
    - `input.isSelectionTriggered()` en lugar de `IsMouseButtonPressed`.
    - `input.isTractorBeamActive()` en lugar de `IsMouseButtonDown`.
3.  **Prioridad**: La UI siempre tiene prioridad. Si el mouse está capturado, las funciones anteriores devolverán `false`, protegiendo el mundo.

## 🧬 Protocolo de Nuevos Elementos
Al añadir un nuevo elemento en `ChemistryDatabase.cpp`:
- **Color Distintivo**: Obligatorio para radar y etiquetas.
- **Sincronización**: El color se propaga al HUD y al Inspector automáticamente.

## 🛠️ Sistema de Widgets (`UIWidgets`)

Para mantener la estética **Nano-HD**, usamos la clase `UIWidgets`. Esta clase automatiza el layout y la interacción.

### Componentes Disponibles

1.  **`drawPanel` & `drawHeader`**: Crean el contenedor base.
    ```cpp
    UIWidgets::drawPanel(rect, input, element.color);
    UIWidgets::drawHeader(rect, "TÍTULO", element.color);
    ```

2.  **`drawValueLabel`**: Ideal para pares Clave-Valor (ID, Masa, etc.). Alinea el valor automáticamente a la derecha.
    ```cpp
    UIWidgets::drawValueLabel(rect, "Masa Atómica", "1.008 u", scrollOffset);
    ```

3.  **`drawTextWrapped`**: Dibuja texto largo con ajuste de línea automático dentro de un ancho fijo.
    ```cpp
    UIWidgets::drawTextWrapped("Lore muy largo...", posX, posY, anchoMax, fontSize, color);
    ```

4.  **`drawButton`**: Botón interactivo con hover y detección de click integrada.
    ```cpp
    if (UIWidgets::drawButton(rect, "OK", input)) { /* Acción */ }
    ```

5.  **`drawSeparator`**: Línea sutil para dividir secciones de contenido.

### Protocolo de Implementación
Cualquier nueva ventana **debe** seguir este orden:
1.  Recibir referencia de `InputHandler`.
2.  Llamar a `drawPanel` (esto activa la captura de mouse).
3.  Llamar a `drawHeader`.
4.  Renderizar botones y etiquetas usando los offsets del rect.
