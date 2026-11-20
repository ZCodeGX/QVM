#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

#define ITERATIONS 100000000

extern qvm_error qvm_runtime_execute_fast(qvm_runtime *rt);

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

static double benchmark_registers_fast(void) {
    uint8_t d[64];
    uint32_t o = 0;
    uint32_t loop_start = 0;

    loop_start = o;

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

    d[o++] = QVM_OP_RETURN; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 0;

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    
    qvm_module *mod = (qvm_module *)calloc(1, sizeof(qvm_module));
    mod->bytecode = (uint8_t *)malloc(o);
    memcpy((uint8_t *)mod->bytecode, d, o);
    mod->bytecode_size = o;
    
    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);

    qvm_type_id_t types[4] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_frame *frame = qvm_frame_create(0, 4, types, NULL, &err);

    qvm_register_write(frame, 0, qvm_value_make(QVM_TYPE_I32, ITERATIONS));
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 1));
    qvm_register_write(frame, 2, qvm_value_make(QVM_TYPE_I32, 0));

    rt->call_stack[0] = frame;
    rt->call_depth = 1;
    rt->pc = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = qvm_runtime_execute_fast(rt);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    qvm_value res;
    qvm_register_read(frame, 2, &res);

    qvm_runtime_destroy(rt);
    free((void *)mod->bytecode);
    free(mod);
    qvm_context_destroy(ctx);

    printf("  Resultado: %lld (esperado: %d)\n", (long long)res.payload, ITERATIONS);
    printf("  Tiempo:    %.4f s\n", elapsed);
    printf("  Rendimiento: %.2f M iter/s\n\n", ITERATIONS / 1e6 / elapsed);

    return elapsed;
}

static double benchmark_qms_zero_fast(void) {
    uint8_t d[128];
    uint32_t o = 0;
    uint32_t loop_start = 0;

    loop_start = o;

    d[o++] = QVM_OP_LOAD_PTR; d[o++] = 3; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    d[o++] = QVM_OP_ARITH_ADD; d[o++] = 3;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 3; d[o++] = 2;

    d[o++] = QVM_OP_STORE_PTR; d[o++] = 0; d[o++] = 3;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    d[o++] = QVM_OP_LOAD_PTR; d[o++] = 3; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    d[o++] = QVM_OP_CMP_LT; d[o++] = 4;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 3; d[o++] = 1;

    d[o++] = QVM_OP_JMP_IF; d[o++] = 4;
    w16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 4;
    w32(d + o, loop_start); o += 4;

    d[o++] = QVM_OP_RETURN; d[o++] = 3;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 0;

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

    qvm_ptr ptr = { .region_id = QVM_REGION_FRAME, .offset = 0, .size = 4, .type_id = QVM_TYPE_I32 };
    qvm_value ptr_val = { .type_id = QVM_TYPE_PTR };
    memcpy(&ptr_val.payload, &ptr, sizeof(qvm_ptr));
    qvm_register_write(frame, 0, ptr_val);
    
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, ITERATIONS));
    qvm_register_write(frame, 2, qvm_value_make(QVM_TYPE_I32, 1));
    
    int32_t *mem = (int32_t *)(rt->frame_region.base + ptr.offset);
    *mem = 0;

    rt->call_stack[0] = frame;
    rt->call_depth = 1;
    rt->pc = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = qvm_runtime_execute_fast(rt);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    qvm_value res;
    qvm_register_read(frame, 3, &res);

    qvm_runtime_destroy(rt);
    free((void *)mod->bytecode);
    free(mod);
    qvm_context_destroy(ctx);

    printf("  Resultado: %lld (esperado: %d)\n", (long long)res.payload, ITERATIONS);
    printf("  Tiempo:    %.4f s\n", elapsed);
    if (elapsed > 0)
        printf("  Rendimiento: %.2f M iter/s\n\n", ITERATIONS / 1e6 / elapsed);
    else
        printf("  Rendimiento: N/A\n\n");

    return elapsed;
}

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  QVM Benchmark OPTIMIZADO: Registros vs QMS-Zero        ║\n");
    printf("║  %d iteraciones cada uno                                 ║\n", ITERATIONS);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    printf("━━━ BENCHMARK 1: Registros (FAST PATH) ━━━\n");
    double t_reg = benchmark_registers_fast();

    printf("━━━ BENCHMARK 2: QMS-Zero (FAST PATH) ━━━\n");
    double t_qms = benchmark_qms_zero_fast();

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                    RESUMEN OPTIMIZADO                   ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");

    if (t_reg > 0 && t_qms > 0) {
        double ratio = t_qms / t_reg;
        double reg_mis = ITERATIONS / 1e6 / t_reg;
        double qms_mis = ITERATIONS / 1e6 / t_qms;

        printf("║  Registros:  %6.2f M iter/s  (%.4f s)              ║\n", reg_mis, t_reg);
        printf("║  QMS-Zero:   %6.2f M iter/s  (%.4f s)              ║\n", qms_mis, t_qms);
        printf("║  Ratio QMS/Reg: %.2fx                                ║\n", ratio);
        printf("║  Overhead de acceso a memoria: +%.0f%%              ║\n", (ratio - 1.0) * 100.0);
    } else {
        printf("║  Uno de los benchmarks falló.                        ║\n");
    }

    printf("╚══════════════════════════════════════════════════════════╝\n");

    return 0;
}
