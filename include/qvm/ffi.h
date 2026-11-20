#ifndef QVM_FFI_H
#define QVM_FFI_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/types.h"
#include "qvm/value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_frame qvm_frame;

typedef qvm_error (*qvm_native_fn)(qvm_frame *frame, const qvm_value *args, uint32_t arg_count, qvm_value *result);

typedef struct qvm_native_function {
    const char      *name;
    qvm_native_fn    fn;
    qvm_type_id_t    return_type;
    uint32_t         arg_count;
    qvm_type_id_t   *arg_types;
} qvm_native_function;

typedef struct qvm_ffi_registry {
    qvm_native_function *functions;
    uint32_t             count;
    uint32_t             capacity;
} qvm_ffi_registry;

qvm_ffi_registry *qvm_ffi_registry_create(qvm_error *err);
void              qvm_ffi_registry_destroy(qvm_ffi_registry *registry);

qvm_error qvm_ffi_register(qvm_ffi_registry *registry, const qvm_native_function *func);
const qvm_native_function *qvm_ffi_find(const qvm_ffi_registry *registry, const char *name);

#ifdef __cplusplus
}
#endif

#endif
