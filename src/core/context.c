/* QVM core - contexto de ejecucion. Estado principal de una instancia. */
#include <stdlib.h>
#include <string.h>

#include "qvm/context.h"

struct qvm_context {
    qvm_config      config;
    qvm_type_table  types;
};

qvm_context *qvm_context_create(const qvm_config *cfg, qvm_error *err) {
    if (err) *err = QVM_OK;
    qvm_context *ctx = (qvm_context *)calloc(1, sizeof *ctx);
    if (!ctx) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    ctx->config = cfg ? *cfg : qvm_config_default();
    ctx->types.entries = NULL;
    ctx->types.count   = 0;
    return ctx;
}

void qvm_context_destroy(qvm_context *ctx) {
    free(ctx);
}

const qvm_config *qvm_context_config(const qvm_context *ctx) {
    return ctx ? &ctx->config : NULL;
}

const qvm_limits *qvm_context_limits(const qvm_context *ctx) {
    return ctx ? &ctx->config.limits : NULL;
}

qvm_mode qvm_context_mode(const qvm_context *ctx) {
    return ctx ? ctx->config.mode : QVM_MODE_SAFE;
}

const qvm_type_table *qvm_context_types(const qvm_context *ctx) {
    return ctx ? &ctx->types : NULL;
}