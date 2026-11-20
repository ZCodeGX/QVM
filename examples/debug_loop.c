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

    // r0 = 5 (límite)
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 5); o += 4;

    // r1 = 1 (incremento)
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 1;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 1); o += 4;

    // r2 = 0 (contador)
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    uint32_t loop_start = o;
    printf("loop_start = %u\n", loop_start);

    // r2 = r2 + r1
    d[o++] = QVM_OP_ARITH_ADD; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 2; d[o++] = 1;

    // r3 = (r2 < r0)
    d[o++] = QVM_OP_CMP_LT; d[o++] = 3;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 2; d[o++] = 0;

    // JMP_IF r3, loop_start
    d[o++] = QVM_OP_JMP_IF; d[o++] = 3;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 3;
    w32(d + o, loop_start); o += 4;

    // RETURN r2
    d[o++] = QVM_OP_RETURN; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 0;

    printf("Bytecode size: %u bytes\n", o);

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    
    qvm_module *mod = (qvm_module *)calloc(1, sizeof(qvm_module));
    mod->bytecode = (uint8_t *)malloc(o);
    memcpy((uint8_t *)mod->bytecode, d, o);
    mod->bytecode_size = o;
    
    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);

    qvm_type_id_t types[4] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_frame *frame = qvm_frame_create(0, 4, types, NULL, &err);

    rt->call_stack[0] = frame;
    rt->call_depth = 1;
    rt->pc = 0;

    printf("Ejecutando...\n");
    
    // Ejecución manual paso a paso para ver dónde falla
    while (rt->pc < rt->module->bytecode_size) {
        qvm_instruction ins = {0};
        err = qvm_instruction_decode(
            rt->module->bytecode + rt->pc,
            rt->module->bytecode_size - rt->pc,
            &ins);
        
        if (err != QVM_OK) {
            printf("Fallo en decode en PC=%u, error=%d\n", rt->pc, err);
            break;
        }
        
        printf("PC=%u, Opcode=%s\n", rt->pc, qvm_opcode_name(ins.opcode));
        
        // Simular avance de PC como lo hace el runtime
        uint32_t size = 0;
        switch (ins.opcode) {
            case QVM_OP_RETURN_VOID: size = 4; break;
            case QVM_OP_MOV: case QVM_OP_CAST: case QVM_OP_ARITH_NEG: 
            case QVM_OP_LOGIC_NOT: case QVM_OP_RETURN: case QVM_OP_STR_LEN: size = 5; break;
            case QVM_OP_ARITH_ADD: case QVM_OP_ARITH_SUB: case QVM_OP_ARITH_MUL: 
            case QVM_OP_ARITH_DIV: case QVM_OP_ARITH_MOD: case QVM_OP_CMP_EQ: 
            case QVM_OP_CMP_NE: case QVM_OP_CMP_LT: case QVM_OP_CMP_LE: 
            case QVM_OP_CMP_GT: case QVM_OP_CMP_GE: case QVM_OP_LOGIC_AND: 
            case QVM_OP_LOGIC_OR: case QVM_OP_STR_CONCAT: case QVM_OP_STR_CHAR_AT: 
            case QVM_OP_STR_SLICE: case QVM_OP_STR_CMP: size = 6; break;
            case QVM_OP_LOAD_CONST: case QVM_OP_JMP: case QVM_OP_ALLOC: 
            case QVM_OP_LOAD_PTR: case QVM_OP_STORE_PTR: size = 8; break;
            case QVM_OP_FREE: size = 4; break;
            case QVM_OP_JMP_IF: case QVM_OP_JMP_IF_NOT: size = 9; break;
            case QVM_OP_CALL: case QVM_OP_CALL_NATIVE: size = 9 + ins.arg_count; break;
            default: size = 4; break;
        }
        rt->pc += size;
    }

    qvm_runtime_destroy(rt);
    free((void *)mod->bytecode);
    free(mod);
    qvm_context_destroy(ctx);

    return 0;
}
