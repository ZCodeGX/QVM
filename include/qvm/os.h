#ifndef QVM_OS_H
#define QVM_OS_H

#include <stddef.h>
#include <stdint.h>
#include "qvm/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QVM_OS_PROT_NONE  = 0,
    QVM_OS_PROT_READ  = 1,
    QVM_OS_PROT_WRITE = 2,
    QVM_OS_PROT_EXEC  = 4
} qvm_os_prot_t;

/* Reserva memoria (RW por defecto para escritura del JIT/QIT) */
qvm_error qvm_os_memory_reserve(void **out_ptr, size_t size);

/* Cambia la protección de la memoria (ej. a RX después de emitir código) */
qvm_error qvm_os_memory_protect(void *ptr, size_t size, qvm_os_prot_t prot);

/* Libera la memoria reservada */
qvm_error qvm_os_memory_release(void *ptr, size_t size);

/* 
 * A3.4: Macro segura para conversión de puntero de datos a puntero de función.
 * Silencia -Wpedantic en GCC/Clang de forma localizada y segura.
 */
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #define QVM_FUNC_FROM_PTR(ptr, type) ((type)(ptr))
    #pragma GCC diagnostic pop
#else
    /* Fallback estándar para otros compiladores */
    #define QVM_FUNC_FROM_PTR(ptr, type) ((type)(uintptr_t)(ptr))
#endif

#ifdef __cplusplus
}
#endif

#endif // QVM_OS_H
