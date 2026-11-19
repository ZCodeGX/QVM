#ifndef QVM_CONTEXT_H
#define QVM_CONTEXT_H

#include <stdint.h>

#include "qvm/config.h"
#include "qvm/error.h"
#include "qvm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_context qvm_context;

/* Estado del ciclo de vida (QVM TS 5.6, invariante 7 no aplica aqui). */
typedef enum qvm_state {
    QVM_STATE_CREATED = 0,
    QVM_STATE_READY   = 1,
    QVM_STATE_RUNNING = 2,
    QVM_STATE_STOPPED = 3,
    QVM_STATE_FAILED  = 4
} qvm_state;

qvm_context *qvm_context_create(const qvm_config *cfg, qvm_error *err);
void         qvm_context_destroy(qvm_context *ctx);

const qvm_config *qvm_context_config(const qvm_context *ctx);
const qvm_limits *qvm_context_limits(const qvm_context *ctx);
qvm_mode          qvm_context_mode  (const qvm_context *ctx);

/* Tabla de tipos del modulo cargado; NULL si no hay modulo activo. */
const qvm_type_table *qvm_context_types(const qvm_context *ctx);

qvm_state qvm_context_state    (const qvm_context *ctx);
void      qvm_context_set_state(qvm_context *ctx, qvm_state s);

#ifdef __cplusplus
}
#endif

#endif /* QVM_CONTEXT_H */