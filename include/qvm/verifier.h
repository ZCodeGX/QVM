#ifndef QVM_VERIFIER_H
#define QVM_VERIFIER_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/context.h"
#include "qvm/loader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_verifier_result {
    qvm_error error;
    uint32_t  pc;
    char      message[256];
} qvm_verifier_result;

qvm_error qvm_verify_module(const qvm_module *module, const qvm_context *ctx, qvm_verifier_result *result);

#ifdef __cplusplus
}
#endif

#endif
