#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/ffi.h"

static qvm_error native_add(qvm_frame *frame, const qvm_value *args, uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 2) return QVM_ERR_INVALID_ARGUMENT;
    result->type_id = QVM_TYPE_I32;
    result->payload = args[0].payload + args[1].payload;
    return QVM_OK;
}

static qvm_error native_strlen(qvm_frame *frame, const qvm_value *args, uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 1) return QVM_ERR_INVALID_ARGUMENT;
    result->type_id = QVM_TYPE_U32;
    result->payload = args[0].payload;
    return QVM_OK;
}

static void test_registry_create_destroy(void) {
    qvm_error err;
    qvm_ffi_registry *reg = qvm_ffi_registry_create(&err);
    assert(reg != NULL && err == QVM_OK);
    assert(reg->count == 0);
    qvm_ffi_registry_destroy(reg);
}

static void test_registry_create_null_err(void) {
    qvm_ffi_registry *reg = qvm_ffi_registry_create(NULL);
    assert(reg != NULL);
    assert(reg->count == 0);
    qvm_ffi_registry_destroy(reg);
}

static void test_register_function(void) {
    qvm_error err;
    qvm_ffi_registry *reg = qvm_ffi_registry_create(&err);
    assert(reg != NULL);
    
    qvm_type_id_t arg_types[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_native_function func = {
        .name = "add",
        .fn = native_add,
        .return_type = QVM_TYPE_I32,
        .arg_count = 2,
        .arg_types = arg_types
    };
    
    err = qvm_ffi_register(reg, &func);
    assert(err == QVM_OK);
    assert(reg->count == 1);
    
    const qvm_native_function *found = qvm_ffi_find(reg, "add");
    assert(found != NULL);
    assert(strcmp(found->name, "add") == 0);
    assert(found->arg_count == 2);
    
    qvm_ffi_registry_destroy(reg);
}

static void test_find_not_found(void) {
    qvm_error err;
    qvm_ffi_registry *reg = qvm_ffi_registry_create(&err);
    assert(reg != NULL);
    
    const qvm_native_function *found = qvm_ffi_find(reg, "nonexistent");
    assert(found == NULL);
    
    qvm_ffi_registry_destroy(reg);
}

static void test_multiple_functions(void) {
    qvm_error err;
    qvm_ffi_registry *reg = qvm_ffi_registry_create(&err);
    assert(reg != NULL);
    
    qvm_type_id_t add_args[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_native_function add_func = {
        .name = "add",
        .fn = native_add,
        .return_type = QVM_TYPE_I32,
        .arg_count = 2,
        .arg_types = add_args
    };
    
    qvm_type_id_t strlen_args[1] = { QVM_TYPE_U32 };
    qvm_native_function strlen_func = {
        .name = "strlen",
        .fn = native_strlen,
        .return_type = QVM_TYPE_U32,
        .arg_count = 1,
        .arg_types = strlen_args
    };
    
    assert(qvm_ffi_register(reg, &add_func) == QVM_OK);
    assert(qvm_ffi_register(reg, &strlen_func) == QVM_OK);
    assert(reg->count == 2);
    
    assert(qvm_ffi_find(reg, "add") != NULL);
    assert(qvm_ffi_find(reg, "strlen") != NULL);
    
    qvm_ffi_registry_destroy(reg);
}

static void test_register_invalid(void) {
    qvm_error err;
    qvm_ffi_registry *reg = qvm_ffi_registry_create(&err);
    assert(reg != NULL);
    
    assert(qvm_ffi_register(NULL, NULL) == QVM_ERR_INVALID_ARGUMENT);
    
    qvm_native_function bad1 = { .name = NULL, .fn = native_add };
    assert(qvm_ffi_register(reg, &bad1) == QVM_ERR_INVALID_ARGUMENT);
    
    qvm_native_function bad2 = { .name = "x", .fn = NULL };
    assert(qvm_ffi_register(reg, &bad2) == QVM_ERR_INVALID_ARGUMENT);
    
    qvm_ffi_registry_destroy(reg);
}

static void test_null_args(void) {
    assert(qvm_ffi_register(NULL, NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_ffi_find(NULL, "x") == NULL);
    assert(qvm_ffi_find(NULL, NULL) == NULL);
    qvm_ffi_registry_destroy(NULL);
}

int main(void) {
    test_registry_create_destroy();
    test_registry_create_null_err();
    test_register_function();
    test_find_not_found();
    test_multiple_functions();
    test_register_invalid();
    test_null_args();
    printf("test_ffi: OK\n");
    return 0;
}
