#ifndef QVM_ERROR_H
#define QVM_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum qvm_error {
    QVM_OK = 0,
    QVM_ERR_INVALID_ARGUMENT,
    QVM_ERR_OUT_OF_MEMORY,
    QVM_ERR_INVALID_BYTECODE,
    QVM_ERR_VERIFICATION_FAILED,
    QVM_ERR_TYPE_MISMATCH,
    QVM_ERR_STACK_OVERFLOW,
    QVM_ERR_CALL_DEPTH_EXCEEDED,
    QVM_ERR_LIMIT_EXCEEDED,
    QVM_ERR_LOW_LEVEL_DISABLED,
    QVM_ERR_NULL_REFERENCE,
    QVM_ERR_INVALID_CHAR,
    QVM_ERR_INVALID_BOOL,
    QVM_ERR_MODULE_INCOMPATIBLE,
    QVM_ERR_IO,
    QVM_ERR_OUT_OF_BOUNDS,
    QVM_ERR_INTERNAL
} qvm_error;

const char *qvm_error_name(qvm_error err);
const char *qvm_error_message(qvm_error err);

#ifdef __cplusplus
}
#endif

#endif /* QVM_ERROR_H */

// Errores específicos de QMS-Zero (Detectados por el Verifier)
#define QVM_ERR_MEMORY_LEAK       30  // Reasignación de registro OWNED sin FREE previo
#define QVM_ERR_DOUBLE_FREE       31  // Intento de liberar un registro CONSUMED
#define QVM_ERR_UNINITIALIZED_READ  32  // Lectura de registro UNINIT
#define QVM_ERR_UNINITIALIZED_WRITE 33  // Escritura en registro UNINIT
#define QVM_ERR_INVALID_BORROW    34  // Violación de reglas de préstamo (Aliasing XOR Mutación)

// Errores específicos de QMS-Zero
#ifndef QVM_ERR_MEMORY_LEAK
#define QVM_ERR_MEMORY_LEAK       30
#endif
#ifndef QVM_ERR_DOUBLE_FREE
#define QVM_ERR_DOUBLE_FREE       31
#endif
#ifndef QVM_ERR_USE_AFTER_FREE
#define QVM_ERR_USE_AFTER_FREE    32
#endif
#ifndef QVM_ERR_UNINITIALIZED_READ
#define QVM_ERR_UNINITIALIZED_READ  33
#endif
#ifndef QVM_ERR_UNINITIALIZED_WRITE
#define QVM_ERR_UNINITIALIZED_WRITE 34
#endif
#ifndef QVM_ERR_INVALID_BORROW
#define QVM_ERR_INVALID_BORROW    35
#endif
