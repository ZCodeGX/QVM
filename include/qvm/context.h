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

qvm_context *qvm_context_create(const qvm_config *cfg, qvm_error *err);
void         qvm_context_destroy(qvm_context *ctx);

const qvm_config *qvm_context_config(const qvm_context *ctx);
const qvm_limits *qvm_context_limits(const qvm_context *ctx);
qvm_mode          qvm_context_mode  (const qvm_context *ctx);

/* Tabla de tipos del modulo cargado; NULL si no hay modulo activo. */
const qvm_type_table *qvm_context_types(const qvm_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* QVM_CONTEXT_H */