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
    // Bucle simple: contador en memoria de 0 a 5
    uint8_t d[128];
    uint32_t o = 0;

    // r0 = puntero a memoria (offset=0, size=4)
    qvm_ptr ptr = { .region_id = QVM_REGION_FRAME, .offset = 0, .size = 4, .type_id = QVM_TYPE_I32 };
    qvm_value ptr_val = { .type_id = QVM_TYPE_PTR };
    memcpy(&ptr_val.payload, &ptr, sizeof(qvm_ptr));

    // r1 = 5 (límite)
    // r2 = 1 (incremento)
    // r3 = valor leído de memoria
    // r4 = resultado de comparación

    uint32_t loop_start = 0;

    // === INICIO DEL BUCLE ===
    loop_start = o;

    // r3 = LOAD_PTR r0, offset=0
    d[o++] = QVM_OP_LOAD_PTR; d[o++] = 3; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    // r3 = r3 + r2
    d[o++] = QVM_OP_ARITH_ADD; d[o++] = 3;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 3; d[o++] = 2;

    // STORE_PTR r0, r3, offset=0
    d[o++] = QVM_OP_STORE_PTR; d[o++] = 0; d[o++] = 3;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    // r3 = LOAD_PTR r0, offset=0 (releer para comparar)
    d[o++] = QVM_OP_LOAD_PTR; d[o++] = 3; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    // r4 = (r3 < r1)
    d[o++] = QVM_OP_CMP_LT; d[o++] = 4;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 3; d[o++] = 1;

    // JMP_IF r4, loop_start
    d[o++] = QVM_OP_JMP_IF; d[o++] = 4;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 4;
    w32(d + o, loop_start); o += 4;

    // RETURN r3
    d[o++] = QVM_OP_RETURN; d[o++] = 3;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 0;

    printf("Bytecode size: %u bytes\n", o);
    printf("loop_start = %u\n", loop_start);

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    
    qvm_module *mod = (qvm_module *)calloc(1, sizeof(qvm_module));
    mod->bytecode = (uint8_t *)malloc(o);
    memcpy((uint8_t *)mod->bytecode, d, o);
    mod->bytecode_size = o;
    
    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);

    qvm_type_id_t types[5] = {
        QVM_TYPE_PTR, QVM_TYPE_I32, QVM_TYPE_I32,
        QVM_TYPE_I32, QVM_TYPE_BOOL
    };
    qvm_frame *frame = qvm_frame_create(0, 5, types, NULL, &err);

    // Inicializar registros
    qvm_register_write(frame, 0, ptr_val);
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 5));  // limit
    qvm_register_write(frame, 2, qvm_value_make(QVM_TYPE_I32, 1));  // inc
    
    // Inicializar memoria
    int32_t *mem = (int32_t *)(rt->frame_region.base + ptr.offset);
    *mem = 0;
    printf("Memoria inicial: %d\n", *mem);

    rt->call_stack[0] = frame;
    rt->call_depth = 1;
    rt->pc = 0;

    printf("Ejecutando...\n");
    err = qvm_runtime_execute(rt);
    printf("Error: %d\n", err);

    qvm_value res;
    qvm_register_read(frame, 3, &res);
    printf("Resultado: %lld (esperado: 5)\n", (long long)res.payload);
    printf("Memoria final: %d\n", *mem);

    qvm_runtime_destroy(rt);
    free((void *)mod->bytecode);
    free(mod);
    qvm_context_destroy(ctx);

    return 0;
}
