#ifndef QVM_RUNTIME_H
#define QVM_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include "qvm/error.h"
#include "qvm/context.h"
#include "qvm/loader.h"
#include "qvm/frames.h"
#include "qvm/ffi.h"
#include "qvm/memory.h"
#include "qvm/qit.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_code_cache_t qvm_code_cache_t;

typedef struct qvm_runtime {
    qvm_context      *ctx;
    qvm_module       *module;
    
    qvm_frame       **call_stack;
    uint32_t          call_depth;
    uint32_t          max_call_depth;
    
    uint32_t         *return_addresses;
    uint32_t         *return_destinations;
    
    qvm_value         return_value;
    int               has_return;
    
    uint32_t          pc;
    
    qvm_ffi_registry *ffi;
    
    // QMS-Zero: Región de memoria para el frame actual
    qvm_region        frame_region;

    qvm_code_cache_t *code_cache;
    uint32_t         *call_counts;
    size_t            call_counts_size;
} qvm_runtime;

qvm_runtime *qvm_runtime_create(qvm_context *ctx, qvm_module *module, qvm_error *err);
void         qvm_runtime_destroy(qvm_runtime *rt);
qvm_ffi_registry *qvm_runtime_ffi(qvm_runtime *rt);

qvm_error qvm_runtime_execute(qvm_runtime *rt);

/* Ejecuta una función específica con tiering automático */
qvm_error qvm_runtime_execute_function(
    qvm_runtime *rt,
    uint32_t function_index,
    const qvm_value *args,
    uint32_t arg_count,
    qvm_exec_result *result);

#ifdef __cplusplus
}
#endif

#endif // QVM_RUNTIME_H
