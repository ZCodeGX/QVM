#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

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

int main(void) {
    uint8_t d[128];
    uint32_t o = 0;

    // Primera instrucción: LOAD_PTR r3, r0, offset=0
    d[o++] = QVM_OP_LOAD_PTR; d[o++] = 3; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    printf("Bytecode: ");
    for (uint32_t i = 0; i < o; i++) printf("%02x ", d[i]);
    printf("\n");
    printf("QVM_OP_LOAD_PTR = 0x%02x\n", QVM_OP_LOAD_PTR);

    // Intentar decodificar
    qvm_instruction ins = {0};
    qvm_error err = qvm_instruction_decode(d, o, &ins);
    
    printf("Error: %d\n", err);
    printf("Opcode decodificada: 0x%02x (%s)\n", ins.opcode, qvm_opcode_name(ins.opcode));
    printf("dst=%u, src1=%u, type_id=%u, imm32=%u\n", ins.dst, ins.src1, ins.type_id, ins.imm32);

    return 0;
}
