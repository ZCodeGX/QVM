#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "qvm/qvm.h"
#include "qvm/qit.h"

static void w32(uint8_t *b, uint32_t v) {
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
    b[2] = (uint8_t)((v >> 16) & 0xFF);
    b[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void w16(uint8_t *b, uint16_t v) {
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
}

void test_qit_compile_simple(void) {
    printf("Test: Compilación simple...\n");
    
    uint8_t bytecode[] = {
        QVM_OP_RETURN, 0, 0x04, 0x00, 0
    };
    
    qit_set_debug(1);
    qit_code *code = qit_compile(bytecode, sizeof(bytecode));
    qit_set_debug(0);
    
    assert(code != NULL);
    assert(qit_get_entry(code) != NULL);
    assert(qit_get_size(code) > 0);
    
    qit_destroy(code);
    printf("  ✓ OK\n\n");
}

void test_qit_execute_arith(void) {
    printf("Test: Ejecución aritmética...\n");
    
    uint8_t bytecode[64];
    uint32_t o = 0;
    
    // r2 = r0 + r1
    bytecode[o++] = QVM_OP_ARITH_ADD; bytecode[o++] = 2;
    w16(bytecode + o, QVM_TYPE_I32); o += 2;
    bytecode[o++] = 0; bytecode[o++] = 1;
    
    // return r2
    bytecode[o++] = QVM_OP_RETURN; bytecode[o++] = 2;
    w16(bytecode + o, QVM_TYPE_I32); o += 2;
    bytecode[o++] = 0;
    
    qit_set_debug(1);
    qit_code *code = qit_compile(bytecode, o);
    qit_set_debug(0);
    
    assert(code != NULL);
    
    // Crear frame
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *frame = qvm_frame_create(0, 3, types, NULL, &err);
    
    qvm_register_write(frame, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 20));
    
    // Ejecutar
    qit_func_t func = (qit_func_t)qit_get_entry(code);
    printf("Ejecutando función compilada...\n");
    int64_t result = func(frame);
    printf("Resultado: %lld\n", (long long)result);
    
    assert(result == 30);
    
    qit_destroy(code);
    qvm_frame_destroy(frame);
    qvm_context_destroy(ctx);
    printf("  ✓ OK\n\n");
}

int main(void) {
    printf("=== Tests QIT ===\n\n");
    
    test_qit_compile_simple();
    test_qit_execute_arith();
    
    printf("✓ Todos los tests pasaron\n");
    return 0;
}
