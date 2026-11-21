#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "qvm/context.h"
#include "qvm/runtime.h"
#include "qvm/loader.h"
#include "qvm/config.h"

int main(void) {
    printf("=== Tests Tiering y Caché ===\n\n");
    
    /* Test 1: Verificar que el umbral está definido */
    printf("Test: Umbral de tiering...\n");
    printf("  QVM_CONFIG_JIT_THRESHOLD = %d\n", QVM_CONFIG_JIT_THRESHOLD);
    assert(QVM_CONFIG_JIT_THRESHOLD > 0);
    printf("  ✓ OK\n\n");
    
    /* Test 2: Crear runtime con caché */
    printf("Test: Runtime con caché...\n");
    qvm_error err;
    qvm_config cfg = {0};
    cfg.limits.max_call_depth = QVM_CONFIG_MAX_CALL_DEPTH;
    
    qvm_context *ctx = qvm_context_create(&cfg, &err);
    assert(ctx != NULL);
    
    /* Crear un módulo dummy */
    qvm_module *module = calloc(1, sizeof(qvm_module));
    module->id = 1;
    module->version = 1;
    module->function_count = 1;
    
    qvm_runtime *rt = qvm_runtime_create(ctx, module, &err);
    assert(rt != NULL);
    assert(rt->code_cache != NULL);
    assert(rt->call_counts != NULL);
    printf("  Caché inicializada: %p\n", (void *)rt->code_cache);
    printf("  Contadores inicializados: %p\n", (void *)rt->call_counts);
    printf("  ✓ OK\n\n");
    
    /* Test 3: Ejecutar función (debería usar intérprete al principio) */
    printf("Test: Ejecución con tiering...\n");
    qvm_exec_result result = {0};
    err = qvm_runtime_execute_function(rt, 0, NULL, 0, &result);
    assert(err == QVM_OK);
    printf("  Primera ejecución: contador = %u\n", rt->call_counts[0]);
    assert(rt->call_counts[0] == 1);
    printf("  ✓ OK\n\n");
    
    /* Cleanup */
    qvm_runtime_destroy(rt);
    qvm_context_destroy(ctx);
    free(module);
    
    printf("✓ Todos los tests de tiering pasaron\n");
    return 0;
}
