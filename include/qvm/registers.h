#ifndef QVM_REGISTERS_H
#define QVM_REGISTERS_H

#include <stdint.h>

#include "qvm/error.h"
#include "qvm/frames.h"
#include "qvm/value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef qvm_value qvm_register;

/* Acceso tipado. Falla si el indice esta fuera de rango o el registro es void. */
qvm_error qvm_register_read (const qvm_frame *frame, uint32_t index, qvm_value *out);
qvm_error qvm_register_write(qvm_frame *frame, uint32_t index, qvm_value v);

/* Par (i128/u128): index debe ser par; (index, index+1) contiguos. */
qvm_error qvm_register_read_pair (const qvm_frame *frame, uint32_t index, uint64_t *lo, uint64_t *hi);
qvm_error qvm_register_write_pair(qvm_frame *frame, uint32_t index, uint64_t lo, uint64_t hi);

#ifdef __cplusplus
}
#endif

#endif /* QVM_REGISTERS_H */