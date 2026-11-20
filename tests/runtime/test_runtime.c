#include <assert.h>
#include <stdio.h>
#include "qvm/qvm.h"
#include "qvm/runtime.h"

static void test_runtime_create_destroy(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    qvm_module mod = {0};
    mod.bytecode = NULL;
    mod.bytecode_size = 0;
    
    qvm_runtime *rt = qvm_runtime_create(ctx, &mod, &err);
    assert(rt != NULL && err == QVM_OK);
    
    qvm_runtime_destroy(rt);
    qvm_context_destroy(ctx);
}

static void test_simple_execution(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    
    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *frame = qvm_frame_create(0, 3, types, NULL, &err);
    assert(frame != NULL);
    
    qvm_register_write(frame, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 20));
    
    uint8_t bytecode[] = {
        QVM_OP_ARITH_ADD, 2, QVM_TYPE_I32 & 0xFF, (QVM_TYPE_I32 >> 8) & 0xFF, 0, 1
    };
    
    qvm_module mod = {0};
    mod.bytecode = bytecode;
    mod.bytecode_size = sizeof(bytecode);
    
    qvm_runtime *rt = qvm_runtime_create(ctx, &mod, &err);
    assert(rt != NULL);
    rt->call_stack[0] = frame;
    rt->call_depth = 1;
    
    err = qvm_runtime_execute(rt);
    assert(err == QVM_OK);
    
    qvm_value result;
    qvm_register_read(frame, 2, &result);
    assert(result.payload == 30);
    
    qvm_runtime_destroy(rt);
    qvm_context_destroy(ctx);
}

int main(void) {
    test_runtime_create_destroy();
    test_simple_execution();
    printf("test_runtime: OK\n");
    return 0;
}
