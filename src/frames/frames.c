/* QVM frames - creacion y ciclo de vida. Sin ejecucion. */
#include <stdlib.h>
#include <string.h>

#include "qvm/frames.h"

/* BLINDAJE ABI FASE 1 */
_Static_assert(sizeof(qvm_value) == 16, "ABI CRITICA: qvm_value debe ser exactamente 16 bytes");

struct qvm_frame {
    uint32_t           function_index;
    uint32_t           register_count;
    const qvm_frame   *caller;
    qvm_type_id_t     *register_types;
    qvm_value         *registers;
};

qvm_frame *qvm_frame_create(uint32_t              function_index,
                            uint32_t              register_count,
                            const qvm_type_id_t  *register_types,
                            const qvm_frame      *caller,
                            qvm_error            *err) {
    if (err) *err = QVM_OK;
    if (register_count == 0 || !register_types) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    qvm_frame *f = (qvm_frame *)calloc(1, sizeof *f);
    if (!f) { if (err) *err = QVM_ERR_OUT_OF_MEMORY; return NULL; }

    f->register_types = (qvm_type_id_t *)malloc(register_count * sizeof(qvm_type_id_t));
    f->registers      = (qvm_value *)calloc(register_count, sizeof(qvm_value));
    if (!f->register_types || !f->registers) {
        free(f->register_types);
        free(f->registers);
        free(f);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }

    for (uint32_t i = 0; i < register_count; ++i) {
        f->register_types[i] = register_types[i];
        f->registers[i]      = qvm_value_make(register_types[i], 0);
    }
    f->function_index = function_index;
    f->register_count = register_count;
    f->caller         = caller;
    return f;
}

void qvm_frame_destroy(qvm_frame *frame) {
    if (!frame) return;
    free(frame->register_types);
    free(frame->registers);
    free(frame);
}

uint32_t qvm_frame_register_count(const qvm_frame *f) { return f ? f->register_count : 0; }
uint32_t qvm_frame_function_index(const qvm_frame *f) { return f ? f->function_index : 0; }
const qvm_type_id_t *qvm_frame_register_types(const qvm_frame *f) { return f ? f->register_types : NULL; }
qvm_value           *qvm_frame_registers      (qvm_frame *f)       { return f ? f->registers       : NULL; }
const qvm_value     *qvm_frame_registers_const(const qvm_frame *f) { return f ? f->registers       : NULL; }
/* QVM_ABI_FRAME_OFFSET: contrato con el prólogo de QIT (mov rdi,[rdi+24]). */
#include <stddef.h>
#define QVM_ABI_FRAME_OFFSET 1
_Static_assert(offsetof(struct qvm_frame, registers) == 24,
               "ABI QIT: el puntero de registros debe estar en offset 24");
