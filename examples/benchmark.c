#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qvm/qvm.h"

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

static void emit_const_i32(uint8_t *d, uint32_t *o, int32_t val) {
    w16(d + *o, QVM_TYPE_I32); *o += 2;
    uint64_t payload = (uint64_t)(int64_t)val;
    memcpy(d + *o, &payload, 8); *o += 8;
}

int main(void) {
    uint8_t d[4096];
    uint32_t o = 0;

    w32(d + o, QCB_MAGIC); o += 4;
    w32(d + o, QCB_VERSION_1_0); o += 4;
    w32(d + o, 0); o += 4;
    w32(d + o, 2); o += 4;

    w32(d + o, QCB_SECTION_CONSTANTS); o += 4;
    uint32_t const_size_offset = o;
    w32(d + o, 0); o += 4;
    uint32_t const_content_start = o;
    
    emit_const_i32(d, &o, 100000000);
    emit_const_i32(d, &o, 1);
    emit_const_i32(d, &o, 0);
    
    w32(d + const_size_offset, o - const_content_start);

    w32(d + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 1;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 1); o += 4;

    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 2); o += 4;

    uint32_t loop_start = o - start;

    d[o++] = QVM_OP_ARITH_ADD; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 2; d[o++] = 1;

    d[o++] = QVM_OP_CMP_LT; d[o++] = 3;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 2; d[o++] = 0;

    d[o++] = QVM_OP_JMP_IF; d[o++] = 3;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 3;
    w32(d + o, loop_start); o += 4;

    d[o++] = QVM_OP_RETURN_VOID;
    d[o++] = 0; d[o++] = 0; d[o++] = 0;

    w32(d + size_at, o - start);

    printf("=== QVM Benchmark: Tight Loop ===\n\n");
    printf("Bytecode size: %u bytes\n", o - start);
    printf("Ejecutando 100,000,000 iteraciones...\n\n");

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    qvm_module *mod = qvm_module_load_from_memory(d, o, ctx, &err);
    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);

    qvm_type_id_t types[4] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_frame *frame = qvm_frame_create(0, 4, types, NULL, &err);

    rt->call_stack[0] = frame;
    rt->call_depth = 1;
    rt->pc = 0;

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    err = qvm_runtime_execute(rt);

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    if (err != QVM_OK) {
        printf("ERROR: %s\n", qvm_error_message(err));
    } else {
        qvm_value res;
        qvm_register_read(frame, 2, &res);
        printf("Resultado final: %lld\n", (long long)res.payload);
        printf("Tiempo total: %.4f segundos\n", elapsed);
        printf("Rendimiento: %.2f millones de iteraciones/segundo\n", 100.0 / elapsed);
    }

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
    return 0;
}
