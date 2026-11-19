#ifndef QVM_FRAMES_H
#define QVM_FRAMES_H

#include <stdint.h>

#include "qvm/error.h"
#include "qvm/types.h"
#include "qvm/value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_frame qvm_frame;

/* Crea un frame. Copia register_types. Registros inicializados a cero del tipo. */
qvm_frame *qvm_frame_create(uint32_t              function_index,
                            uint32_t              register_count,
                            const qvm_type_id_t  *register_types,
                            const qvm_frame      *caller,
                            qvm_error            *err);

void qvm_frame_destroy(qvm_frame *frame);

uint32_t             qvm_frame_register_count (const qvm_frame *f);
uint32_t             qvm_frame_function_index (const qvm_frame *f);
const qvm_type_id_t *qvm_frame_register_types (const qvm_frame *f);
qvm_value           *qvm_frame_registers       (qvm_frame *f);
const qvm_value     *qvm_frame_registers_const (const qvm_frame *f);

#ifdef __cplusplus
}
#endif

#endif /* QVM_FRAMES_H */