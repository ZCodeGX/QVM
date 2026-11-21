#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"
#include "qvm/jit.h"

#define ITERATIONS 100000000

typedef int64_t (*qvm_jit_func)(void *frame, void *runtime);

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

static double benchmark_c_pure(void) {
    volatile int32_t counter = 0;
    volatile int32_t limit = ITERATIONS;
    
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    
    while (counter < limit) {
        counter++;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    
    printf("  Resultado: %d\n", (int32_t)counter);
    printf("  Tiempo:    %.4f s\n", elapsed);
    printf("  Rendimiento: %.2f M iter/s\n\n", ITERATIONS / 1e6 / elapsed);
    
    return elapsed;
}

static double benchmark_jit(void) {
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

    printf("  Compilando JIT...\n");
    qvm_jit_code *jit = qvm_jit_compile(d, o);
    if (!jit) {
        printf("  ERROR: Fallo al compilar JIT\n");
        return -1;
    }
    printf("  JIT compilado exitosamente en %zu bytes\n", jit->size);

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    qvm_type_id_t types[4] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_frame *frame = qvm_frame_create(0, 4, types, NULL, &err);

    qvm_register_write(frame, 0, qvm_value_make(QVM_TYPE_I32, ITERATIONS));
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 1));
    qvm_register_write(frame, 2, qvm_value_make(QVM_TYPE_I32, 0));

    qvm_jit_func func;
    memcpy(&func, &jit->code, sizeof func);
    
    printf("  Ejecutando código JIT...\n");
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    
    // Usar volatile para evitar que el compilador optimice la llamada
    volatile int64_t result = func(frame, NULL);
    
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("  Resultado: %lld\n", (long long)result);
    printf("  Tiempo:    %.4f s\n", elapsed);
    printf("  Rendimiento: %.2f M iter/s\n\n", ITERATIONS / 1e6 / elapsed);

    qvm_jit_destroy(jit);
    qvm_frame_destroy(frame);
    qvm_context_destroy(ctx);

    return elapsed;
}

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  QVM Benchmark: C vs JIT                                ║\n");
    printf("║  %d iteraciones                                          ║\n", ITERATIONS);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    printf("━━━ 1. C PURO (referencia) ━━━\n");
    double t_c = benchmark_c_pure();

    printf("━━━ 2. JIT COMPILADO ━━━\n");
    double t_jit = benchmark_jit();

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                    RESUMEN                              ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");

    if (t_c > 0 && t_jit > 0) {
        double c_mis = ITERATIONS / 1e6 / t_c;
        double jit_mis = ITERATIONS / 1e6 / t_jit;

        printf("║  C puro:        %6.2f M iter/s  (%.4f s)              ║\n", c_mis, t_c);
        printf("║  JIT:           %6.2f M iter/s  (%.4f s)              ║\n", jit_mis, t_jit);
        printf("║  JIT/C ratio:   %.2fx                                   ║\n", t_jit / t_c);
    } else {
        printf("║  Error en la ejecución.                                ║\n");
    }

    printf("╚══════════════════════════════════════════════════════════╝\n");

    return 0;
}
