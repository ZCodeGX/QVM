#ifndef QVM_MEMORY_H
#define QVM_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Identificadores de las regiones de memoria en QMS-Zero
typedef enum qvm_region_id {
    QVM_REGION_GLOBAL = 0,  // Datos que viven toda la ejecución del contexto
    QVM_REGION_MODULE = 1,  // Datos del módulo cargado (bytecode, constantes)
    QVM_REGION_FRAME  = 2,  // Datos locales del frame actual (se limpia en O(1) al retornar)
    QVM_REGION_COUNT  = 3
} qvm_region_id;

// Estructura de una Región de Memoria.
// En QMS-Zero, las regiones usan Bump Allocation para asignación O(1) y limpieza O(1).
typedef struct qvm_region {
    qvm_region_id id;
    uint8_t      *base;         // Puntero crudo base de la región en la RAM real
    uint32_t      capacity;     // Tamaño total asignado en bytes
    uint32_t      offset;       // Offset actual (puntero de asignación)
} qvm_region;

// El Puntero QMS-Zero.
// En el Verifier, esto es un tipo lineal con estado (Owned, Consumed, Borrowed).
// En el Runtime, esto se resuelve a un puntero crudo de C (base + offset) para 100% velocidad.
typedef struct qvm_ptr {
    qvm_region_id region_id;    // A qué región pertenece
    uint32_t      offset;       // Desplazamiento desde el base de la región
    uint32_t      size;         // Tamaño exacto del bloque asignado
    uint16_t      type_id;      // Tipo de dato almacenado (para evitar casts ilegales)
} qvm_ptr;

// Funciones de gestión de regiones (Inicialización y limpieza)
void qvm_region_init(qvm_region *region, qvm_region_id id, uint32_t capacity);
void qvm_region_destroy(qvm_region *region);

// Funciones de asignación (Solo usadas por el Runtime/Loader, no por el usuario final directamente)
// Devuelve un qvm_ptr válido. Si no hay espacio, el Verifier ya debió haberlo rechazado.
qvm_ptr qvm_region_alloc(qvm_region *region, uint32_t size, uint16_t type_id);

// Funciones de acceso directo (Zero-Overhead).
// Estas funciones NO hacen comprobaciones de límites. El Verifier garantiza que son seguras.
static inline void* qvm_ptr_to_raw(const qvm_region *region, const qvm_ptr *ptr) {
    return region->base + ptr->offset;
}

static inline void qvm_ptr_read(const qvm_region *region, const qvm_ptr *ptr, void *dst, uint32_t len) {
    // Cero checks. 100% velocidad de C.
    __builtin_memcpy(dst, region->base + ptr->offset, len);
}

static inline void qvm_ptr_write(qvm_region *region, const qvm_ptr *ptr, const void *src, uint32_t len) {
    // Cero checks. 100% velocidad de C.
    __builtin_memcpy(region->base + ptr->offset, src, len);
}

#ifdef __cplusplus
}
#endif

#endif // QVM_MEMORY_H
