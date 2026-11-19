/* QVM core - ciclo de vida. Transiciones CREATED -> READY -> RUNNING -> STOPPED. */
#include "qvm/lifecycle.h"

qvm_error qvm_init(qvm_context *ctx) {
    if (!ctx) return QVM_ERR_INVALID_ARGUMENT;
    if (qvm_context_state(ctx) != QVM_STATE_CREATED) return QVM_ERR_INVALID_ARGUMENT;
    qvm_context_set_state(ctx, QVM_STATE_READY);
    return QVM_OK;
}

qvm_error qvm_start(qvm_context *ctx) {
    if (!ctx) return QVM_ERR_INVALID_ARGUMENT;
    if (qvm_context_state(ctx) != QVM_STATE_READY) return QVM_ERR_INVALID_ARGUMENT;
    qvm_context_set_state(ctx, QVM_STATE_RUNNING);
    return QVM_OK;
}

qvm_error qvm_stop(qvm_context *ctx) {
    if (!ctx) return QVM_ERR_INVALID_ARGUMENT;
    qvm_state s = qvm_context_state(ctx);
    if (s != QVM_STATE_RUNNING && s != QVM_STATE_READY) return QVM_ERR_INVALID_ARGUMENT;
    qvm_context_set_state(ctx, QVM_STATE_STOPPED);
    return QVM_OK;
}

qvm_error qvm_reset(qvm_context *ctx) {
    if (!ctx) return QVM_ERR_INVALID_ARGUMENT;
    qvm_context_set_state(ctx, QVM_STATE_CREATED);
    return QVM_OK;
}

qvm_state qvm_state_of(const qvm_context *ctx) {
    return qvm_context_state(ctx);
}