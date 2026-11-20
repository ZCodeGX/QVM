#ifndef QVM_RUNTIME_H
#define QVM_RUNTIME_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/context.h"
#include "qvm/frames.h"
#include "qvm/instruction.h"
#include "qvm/value.h"
#include "qvm/loader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_runtime {
    qvm_context     *ctx;
    qvm_module      *module;
    qvm_frame      **call_stack;
    uint32_t         call_depth;
    uint32_t         max_call_depth;
    uint32_t         pc;
} qvm_runtime;

qvm_runtime *qvm_runtime_create(qvm_context *ctx, qvm_module *module, qvm_error *err);
void         qvm_runtime_destroy(qvm_runtime *rt);
qvm_error    qvm_runtime_execute(qvm_runtime *rt);

#ifdef __cplusplus
}
#endif

#endif
