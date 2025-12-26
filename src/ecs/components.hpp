#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

/**
 * REPRESENTACIÓN DE DATOS (ECS)
 * Buscamos el "Mínimo de Datos Requeridos" para máxima velocidad.
 */

// Posición, Velocidad y Rotación
struct TransformComponent {
    float x, y, z;
    float vx, vy, vz;
    float rotation;
};

// Datos del Átomo
struct AtomComponent {
    int atomicNumber;  // Solo el ID único. El resto se busca en ChemistryDatabase.
    float partialCharge;
};

// Estado de Agrupación (Moleculas)
struct StateComponent {
    bool isClustered;
    int moleculeId;      // -1 si no está en una molécula estable
    int parentEntityId;  // A quién está anclado (-1 = root)
    int parentSlotIndex; // Qué brazo del padre ocupa
    float dockingProgress; // 0.0 = recien enlazado, 1.0 = totalmente acoplado
    bool isShielded;     // Bloquea cualquier enlace espontáneo (Rayo Tractor)
};

#endif
