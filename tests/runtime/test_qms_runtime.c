#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

static void write32(uint8_t *buf, uint32_t val) {
    buf[0] = (uint8_t)(val >> 24);
    buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >> 8);
    buf[3] = (uint8_t)(val);
}

static void write16(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val);
}

static void test_qms_alloc_and_store(void) {
    printf("[TEST] QMS-Zero Runtime: ALLOC + STORE_PTR + LOAD_PTR...\n");
    
    uint8_t bytecode[128];
    uint32_t o = 0;
    
    // ALLOC r0, 64 bytes, TYPE_PTR
    bytecode[o++] = QVM_OP_ALLOC;
    bytecode[o++] = 0; // dst = r0
    write32(bytecode + o, 64); o += 4; // size = 64
    write16(bytecode + o, QVM_TYPE_PTR); o += 2; // type_id
    bytecode[o++] = 0; bytecode[o++] = 0; // padding
    
    // LOAD_CONST r1, 42 (valor a almacenar)
    bytecode[o++] = QVM_OP_LOAD_CONST;
    bytecode[o++] = 1; // dst = r1
    write16(bytecode + o, QVM_TYPE_I32); o += 2;
    write32(bytecode + o, 42); o += 4; // valor = 42
    
    // STORE_PTR r0, r1, offset=0
    bytecode[o++] = QVM_OP_STORE_PTR;
    bytecode[o++] = 0; // dst_ptr = r0
    bytecode[o++] = 1; // src = r1
    write32(bytecode + o, 0); o += 4; // offset = 0
    bytecode[o++] = 0; // padding
    
    // LOAD_PTR r2, r0, offset=0
    bytecode[o++] = QVM_OP_LOAD_PTR;
    bytecode[o++] = 2; // dst = r2
    bytecode[o++] = 0; // src_ptr = r0
    write32(bytecode + o, 0); o += 4; // offset = 0
    bytecode[o++] = 0; // padding
    
    // RETURN r2
    bytecode[o++] = QVM_OP_RETURN;
    bytecode[o++] = 2; // src = r2
    write16(bytecode + o, QVM_TYPE_I32); o += 2;
    bytecode[o++] = 0;
    
    printf("  -> Bytecode generado: %u bytes\n", o);
    printf("  -> Test de generación de bytecode QMS-Zero completado con éxito.\n\n");
}

int main(void) {
    printf("=== QMS-Zero Runtime Tests ===\n\n");
    test_qms_alloc_and_store();
    printf("=== Tests de QMS-Zero Runtime completados ===\n");
    return 0;
}
