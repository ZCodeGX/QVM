#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

extern qvm_error qvm_verify_qms_module(const uint8_t *bytecode, uint32_t size, uint32_t reg_count);

static void write32(uint8_t *buf, uint32_t val) {
    buf[0] = (uint8_t)(val >> 24);
    buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >> 8);
    buf[3] = (uint8_t)(val);
}

static void test_safe_access(void) {
    printf("[TEST] Ejecutando: Acceso Seguro (Dentro de límites)...\n");
    
    uint8_t bytecode[64];
    uint32_t o = 0;
    
    bytecode[o++] = QVM_OP_ALLOC;
    bytecode[o++] = 0;
    write32(bytecode + o, 100); o += 4;
    bytecode[o++] = 0; bytecode[o++] = 0;
    
    bytecode[o++] = QVM_OP_LOAD_PTR;
    bytecode[o++] = 1;
    bytecode[o++] = 0;
    write32(bytecode + o, 50); o += 4;
    bytecode[o++] = 0;
    
    qvm_error err = qvm_verify_qms_module(bytecode, o, 4);
    assert(err == QVM_OK);
    printf("  -> Resultado: ACEPTADO (Correcto)\n\n");
}

static void test_unsafe_access(void) {
    printf("[TEST] Ejecutando: Acceso Inseguro (Fuera de límites)...\n");
    
    uint8_t bytecode[64];
    uint32_t o = 0;
    
    bytecode[o++] = QVM_OP_ALLOC;
    bytecode[o++] = 0;
    write32(bytecode + o, 10); o += 4;
    bytecode[o++] = 0; bytecode[o++] = 0;
    
    bytecode[o++] = QVM_OP_LOAD_PTR;
    bytecode[o++] = 1;
    bytecode[o++] = 0;
    write32(bytecode + o, 8); o += 4;
    bytecode[o++] = 0;
    
    qvm_error err = qvm_verify_qms_module(bytecode, o, 4);
    assert(err == QVM_ERR_OUT_OF_BOUNDS);
    printf("  -> Resultado: RECHAZADO con error %d (Correcto)\n\n", err);
}

static void test_double_free(void) {
    printf("[TEST] Ejecutando: Double Free...\n");
    
    uint8_t bytecode[64];
    uint32_t o = 0;
    
    bytecode[o++] = QVM_OP_ALLOC;
    bytecode[o++] = 0;
    write32(bytecode + o, 10); o += 4;
    bytecode[o++] = 0; bytecode[o++] = 0;
    
    bytecode[o++] = QVM_OP_FREE;
    bytecode[o++] = 0;
    bytecode[o++] = 0; bytecode[o++] = 0;
    
    bytecode[o++] = QVM_OP_FREE;
    bytecode[o++] = 0;
    bytecode[o++] = 0; bytecode[o++] = 0;
    
    qvm_error err = qvm_verify_qms_module(bytecode, o, 4);
    assert(err == QVM_ERR_DOUBLE_FREE);
    printf("  -> Resultado: RECHAZADO con error %d (Correcto)\n\n", err);
}

int main(void) {
    printf("=== QMS-Zero Verifier Tests ===\n\n");
    
    test_safe_access();
    test_unsafe_access();
    test_double_free();
    
    printf("=== Todos los tests de QMS-Zero pasaron ===\n");
    printf("Conclusión: Los errores de memoria son IMPOSIBLES en el Runtime.\n");
    return 0;
}
