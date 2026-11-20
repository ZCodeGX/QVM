#include <stdlib.h>
#include <string.h>
#include "qvm/ffi.h"

qvm_ffi_registry *qvm_ffi_registry_create(qvm_error *err) {
    if (err) *err = QVM_OK;
    qvm_ffi_registry *registry = (qvm_ffi_registry *)calloc(1, sizeof *registry);
    if (!registry) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    registry->capacity = 16;
    registry->functions = (qvm_native_function *)malloc(registry->capacity * sizeof(qvm_native_function));
    if (!registry->functions) {
        free(registry);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    return registry;
}

void qvm_ffi_registry_destroy(qvm_ffi_registry *registry) {
    if (!registry) return;
    if (registry->functions) {
        for (uint32_t i = 0; i < registry->count; ++i) {
            free(registry->functions[i].arg_types);
        }
        free(registry->functions);
    }
    free(registry);
}

qvm_error qvm_ffi_register(qvm_ffi_registry *registry, const qvm_native_function *func) {
    if (!registry || !func || !func->name || !func->fn) return QVM_ERR_INVALID_ARGUMENT;
    
    if (registry->count >= registry->capacity) {
        uint32_t new_cap = registry->capacity * 2;
        qvm_native_function *new_funcs = (qvm_native_function *)realloc(
            registry->functions, new_cap * sizeof(qvm_native_function));
        if (!new_funcs) return QVM_ERR_OUT_OF_MEMORY;
        registry->functions = new_funcs;
        registry->capacity = new_cap;
    }
    
    qvm_native_function *dst = &registry->functions[registry->count];
    dst->name = func->name;
    dst->fn = func->fn;
    dst->return_type = func->return_type;
    dst->arg_count = func->arg_count;
    
    if (func->arg_count > 0 && func->arg_types) {
        dst->arg_types = (qvm_type_id_t *)malloc(func->arg_count * sizeof(qvm_type_id_t));
        if (!dst->arg_types) return QVM_ERR_OUT_OF_MEMORY;
        memcpy(dst->arg_types, func->arg_types, func->arg_count * sizeof(qvm_type_id_t));
    } else {
        dst->arg_types = NULL;
    }
    
    registry->count++;
    return QVM_OK;
}

const qvm_native_function *qvm_ffi_find(const qvm_ffi_registry *registry, const char *name) {
    if (!registry || !name) return NULL;
    for (uint32_t i = 0; i < registry->count; ++i) {
        if (strcmp(registry->functions[i].name, name) == 0) {
            return &registry->functions[i];
        }
    }
    return NULL;
}
