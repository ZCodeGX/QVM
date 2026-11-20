#ifndef QVM_MEMORY_VERIFIER_H
#define QVM_MEMORY_VERIFIER_H

#include "qvm/memory.h"
#include "qvm/error.h"

#ifdef __cplusplus
extern "C" {
#endif

// Estados de un puntero lineal en el Verifier
typedef enum qvm_ptr_state {
    QVM_PTR_STATE_UNINIT = 0, // Registro no inicializado
    QVM_PTR_STATE_OWNED  = 1, // Puntero válido, dueño exclusivo
    QVM_PTR_STATE_BORROWED = 2, // Préstamo de lectura (puede haber varios)
    QVM_PTR_STATE_CONSUMED = 3  // Ya fue liberado o movido (imposible de usar)
} qvm_ptr_state;

// Información que el Verifier rastrea por cada registro de la VM
typedef struct qvm_reg_info {
    qvm_ptr_state state;
    qvm_region_id region_id;
    uint32_t      size;       // Tamaño conocido en tiempo de verificación
    uint16_t      type_id;
} qvm_reg_info;

// Contexto del Verificador de Memoria
typedef struct qvm_mem_verifier {
    qvm_reg_info *registers;  // Estado de cada registro
    uint32_t      reg_count;
    
    // Límites de las regiones (para comprobar límites estáticos)
    uint32_t      region_limits[QVM_REGION_COUNT];
} qvm_mem_verifier;

qvm_error qvm_mem_verifier_init(qvm_mem_verifier *v, uint32_t reg_count);
void      qvm_mem_verifier_destroy(qvm_mem_verifier *v);

// Reglas de QMS-Zero
qvm_error qvm_mem_verify_alloc(qvm_mem_verifier *v, uint32_t dst_reg, qvm_region_id region, uint32_t size, uint16_t type_id);
qvm_error qvm_mem_verify_free(qvm_mem_verifier *v, uint32_t src_reg);
qvm_error qvm_mem_verify_load(qvm_mem_verifier *v, uint32_t dst_reg, uint32_t src_reg, uint32_t offset, uint32_t len);
qvm_error qvm_mem_verify_store(qvm_mem_verifier *v, uint32_t dst_reg, uint32_t src_reg, uint32_t offset, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // QVM_MEMORY_VERIFIER_H
