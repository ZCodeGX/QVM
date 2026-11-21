#ifndef QVM_LOADER_H
#define QVM_LOADER_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/context.h"
#include "qvm/value.h"
#include "qvm/types.h"
#include "qvm/qcb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_function {
    uint32_t         bytecode_offset;
    uint16_t         param_count;
    uint16_t         local_count;
    qvm_type_id_t    return_type;
    qvm_type_id_t   *local_types;
} qvm_function;

typedef struct qvm_module {
    uint32_t          id;
    uint32_t          version;
    const uint8_t    *bytecode;
    uint32_t          bytecode_size;
    const qvm_value  *constants;
    uint32_t          constant_count;
    qvm_function     *functions;
    uint32_t          function_count;
    uint8_t          *owned_data;
    qvm_value        *owned_constants;
} qvm_module;

qvm_module *qvm_module_load_from_file(const char *path, qvm_context *ctx, qvm_error *err);
qvm_module *qvm_module_load_from_memory(const uint8_t *data, uint32_t size, qvm_context *ctx, qvm_error *err);
void        qvm_module_destroy(qvm_module *module);

#ifdef __cplusplus
}
#endif

#endif
