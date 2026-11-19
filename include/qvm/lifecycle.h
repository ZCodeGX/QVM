#ifndef QVM_LIFECYCLE_H
#define QVM_LIFECYCLE_H

#include "qvm/context.h"
#include "qvm/error.h"

#ifdef __cplusplus
extern "C" {
#endif

qvm_error qvm_init  (qvm_context *ctx);
qvm_error qvm_start (qvm_context *ctx);
qvm_error qvm_stop  (qvm_context *ctx);
qvm_error qvm_reset (qvm_context *ctx);

qvm_state qvm_state_of(const qvm_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* QVM_LIFECYCLE_H */