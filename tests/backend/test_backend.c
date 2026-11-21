#include <stdio.h>
#include <assert.h>
#include "qvm/backend.h"

int main(void) {
    printf("=== Tests Backend Dispatcher ===\n\n");
    
    /* Test 1: Detección de arquitectura */
    printf("Test: Detección de arquitectura...\n");
    qvm_arch_t arch = qvm_arch_detect();
    printf("  Arquitectura detectada: %s\n", qvm_arch_name(arch));
    assert(arch == QVM_ARCH_X86_64 || arch == QVM_ARCH_AARCH64 || 
           arch == QVM_ARCH_RISCV64 || arch == QVM_ARCH_UNKNOWN);
    printf("  ✓ OK\n\n");
    
    /* Test 2: Lista de backends */
    printf("Test: Lista de backends...\n");
    size_t count = 0;
    const qvm_backend_info_t *backends = qvm_backend_list(&count);
    assert(count > 0);
    printf("  Backends disponibles: %zu\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("    - %s (arch=%s, available=%d)\n", 
               backends[i].name, 
               qvm_arch_name(backends[i].arch),
               backends[i].available);
    }
    printf("  ✓ OK\n\n");
    
    /* Test 3: Selección de backend */
    printf("Test: Selección de backend...\n");
    qvm_backend_id_t selected = qvm_backend_select(arch);
    printf("  Backend seleccionado para %s: %d\n", qvm_arch_name(arch), selected);
    /* En x86_64, debería seleccionar x86_64_sysv */
    if (arch == QVM_ARCH_X86_64) {
        assert(selected == QVM_BACKEND_X86_64_SYSV);
    }
    printf("  ✓ OK\n\n");
    
    printf("✓ Todos los tests de backend pasaron\n");
    return 0;
}
