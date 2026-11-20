#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/verifier.h"

static void test_null_args(void) {
    qvm_verifier_result result;
    assert(qvm_verify_module(NULL, NULL, &result) == QVM_ERR_INVALID_ARGUMENT);
}

static void test_empty_bytecode(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    qvm_module mod = {0};
    mod.bytecode = NULL;
    mod.bytecode_size = 0;
    
    qvm_verifier_result result;
    assert(qvm_verify_module(&mod, ctx, &result) == QVM_ERR_INVALID_BYTECODE);
    
    qvm_context_destroy(ctx);
}

static void test_valid_simple(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    uint8_t bytecode[] = {
        QVM_OP_RETURN_VOID, 0, 0, 0
    };
    
    qvm_module mod = {0};
    mod.bytecode = bytecode;
    mod.bytecode_size = sizeof(bytecode);
    
    qvm_verifier_result result;
    assert(qvm_verify_module(&mod, ctx, &result) == QVM_OK);
    
    qvm_context_destroy(ctx);
}

static void test_invalid_opcode(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    uint8_t bytecode[] = { 0xFF, 0, 0, 0 };
    
    qvm_module mod = {0};
    mod.bytecode = bytecode;
    mod.bytecode_size = sizeof(bytecode);
    
    qvm_verifier_result result;
    assert(qvm_verify_module(&mod, ctx, &result) == QVM_ERR_INVALID_BYTECODE);
    
    qvm_context_destroy(ctx);
}

static void test_jump_out_of_range(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    uint8_t bytecode[] = {
        QVM_OP_JMP, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF
    };
    
    qvm_module mod = {0};
    mod.bytecode = bytecode;
    mod.bytecode_size = sizeof(bytecode);
    
    qvm_verifier_result result;
    assert(qvm_verify_module(&mod, ctx, &result) == QVM_ERR_INVALID_BYTECODE);
    
    qvm_context_destroy(ctx);
}

static void test_constant_out_of_range(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    uint8_t bytecode[] = {
        QVM_OP_LOAD_CONST, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF
    };
    
    qvm_module mod = {0};
    mod.bytecode = bytecode;
    mod.bytecode_size = sizeof(bytecode);
    mod.constants = NULL;
    mod.constant_count = 0;
    
    qvm_verifier_result result;
    assert(qvm_verify_module(&mod, ctx, &result) == QVM_ERR_INVALID_BYTECODE);
    
    qvm_context_destroy(ctx);
}

int main(void) {
    test_null_args();
    test_empty_bytecode();
    test_valid_simple();
    test_invalid_opcode();
    test_jump_out_of_range();
    test_constant_out_of_range();
    printf("test_verifier: OK\n");
    return 0;
}
