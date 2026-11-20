#include <stdlib.h>
#include <string.h>
#include "qvm/memory_verifier.h"

qvm_error qvm_mem_verifier_init(qvm_mem_verifier *v, uint32_t reg_count) {
    if (!v) return QVM_ERR_INVALID_ARGUMENT;
    v->registers = (qvm_reg_info *)calloc(reg_count, sizeof(qvm_reg_info));
    if (!v->registers) return QVM_ERR_OUT_OF_MEMORY;
    v->reg_count = reg_count;
    return QVM_OK;
}

void qvm_mem_verifier_destroy(qvm_mem_verifier *v) {
    if (v && v->registers) {
        free(v->registers);
        v->registers = NULL;
    }
}

// Regla 1: Asignación. El registro destino debe estar libre o ser reasignado (consumiendo el anterior).
qvm_error qvm_mem_verify_alloc(qvm_mem_verifier *v, uint32_t dst_reg, qvm_region_id region, uint32_t size, uint16_t type_id) {
    if (dst_reg >= v->reg_count) return QVM_ERR_INVALID_ARGUMENT;
    if (size == 0) return QVM_ERR_INVALID_ARGUMENT;
    
    // Si el registro ya tenía un puntero OWNED, debemos marcarlo como CONSUMED (fuga de memoria si no se liberó antes).
    // En un lenguaje de alto nivel, el compilador debe insertar un FREE explícito antes de reasignar.
    if (v->registers[dst_reg].state == QVM_PTR_STATE_OWNED) {
        return QVM_ERR_MEMORY_LEAK; // Rechazado: reasignación sin liberar
    }

    // Comprobación estática de límites de la región
    if (region == QVM_REGION_FRAME) {
        // El Verifier no conoce el tamaño exacto del frame en tiempo de carga, 
        // pero puede comprobar que no exceda un límite máximo definido en el módulo.
        // Por ahora, aceptamos cualquier tamaño <= límite de región.
    }

    v->registers[dst_reg].state = QVM_PTR_STATE_OWNED;
    v->registers[dst_reg].region_id = region;
    v->registers[dst_reg].size = size;
    v->registers[dst_reg].type_id = type_id;
    return QVM_OK;
}

// Regla 2: Liberación. Solo se puede liberar un puntero OWNED. Pasa a CONSUMED.
qvm_error qvm_mem_verify_free(qvm_mem_verifier *v, uint32_t src_reg) {
    if (src_reg >= v->reg_count) return QVM_ERR_INVALID_ARGUMENT;
    
    qvm_ptr_state state = v->registers[src_reg].state;
    if (state != QVM_PTR_STATE_OWNED) {
        if (state == QVM_PTR_STATE_CONSUMED) return QVM_ERR_DOUBLE_FREE;
        if (state == QVM_PTR_STATE_BORROWED) return QVM_ERR_INVALID_BORROW;
        return QVM_ERR_USE_AFTER_FREE;
    }

    v->registers[src_reg].state = QVM_PTR_STATE_CONSUMED;
    return QVM_OK;
}

// Regla 3: Lectura (Bounds Checking Elimination).
// El Verifier comprueba matemáticamente que offset + len <= size.
qvm_error qvm_mem_verify_load(qvm_mem_verifier *v, uint32_t dst_reg, uint32_t src_reg, uint32_t offset, uint32_t len) {
    if (src_reg >= v->reg_count || dst_reg >= v->reg_count) return QVM_ERR_INVALID_ARGUMENT;
    
    qvm_reg_info *info = &v->registers[src_reg];
    if (info->state == QVM_PTR_STATE_CONSUMED) return QVM_ERR_USE_AFTER_FREE;
    if (info->state == QVM_PTR_STATE_UNINIT) return QVM_ERR_UNINITIALIZED_READ;

    // COMPROBACIÓN MATEMÁTICA ESTÁTICA (Cero overhead en runtime)
    if (offset + len > info->size) {
        return QVM_ERR_OUT_OF_BOUNDS; // Rechazado por el Verifier
    }

    // La lectura es segura. El registro destino recibe los datos (no es un puntero, es un valor).
    // No cambiamos el estado del puntero fuente (se puede leer múltiples veces).
    return QVM_OK;
}

// Regla 4: Escritura. Mismas reglas que lectura, pero requiere que el puntero no sea un préstamo de solo lectura.
qvm_error qvm_mem_verify_store(qvm_mem_verifier *v, uint32_t dst_reg, uint32_t src_reg, uint32_t offset, uint32_t len) {
    if (src_reg >= v->reg_count || dst_reg >= v->reg_count) return QVM_ERR_INVALID_ARGUMENT;
    
    qvm_reg_info *info = &v->registers[dst_reg]; // Escribimos EN dst_reg
    if (info->state == QVM_PTR_STATE_CONSUMED) return QVM_ERR_USE_AFTER_FREE;
    if (info->state == QVM_PTR_STATE_UNINIT) return QVM_ERR_UNINITIALIZED_WRITE;
    
    // Si fuera un préstamo de lectura (en una implementación futura de borrows), rechazaríamos aquí.
    // Por ahora, si es OWNED, se puede escribir.

    if (offset + len > info->size) {
        return QVM_ERR_OUT_OF_BOUNDS;
    }

    return QVM_OK;
}
