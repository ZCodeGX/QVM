#ifndef QVM_EXECUTION_H
#define QVM_EXECUTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "qvm/types.h"
#include "qvm/value.h"
#include "qvm/error.h"

/* Contexto de ejecución canónico. Es la única fuente de verdad para 
   Intérprete, Fast Path, JIT y QIT. */
typedef struct {
    const qvm_value *registers;
    uint32_t register_count;
    const uint8_t *bytecode;
    uint32_t bytecode_size;
    const qvm_type_id_t *types;
    void *os_env; /* Reservado para extensiones futuras (FFI, OS calls) */
} qvm_exec_context;

/* Resultado estandarizado de una función compilada/ejecutada */
typedef struct {
    qvm_error error;
    int64_t payload;
    qvm_type_id_t type_id;
    uint8_t flags;
} qvm_exec_result;

/* Firma canónica de punto de entrada para backends nativos */
typedef qvm_exec_result (*qvm_entry_fn)(const qvm_exec_context *ctx);

#ifdef __cplusplus
}
#endif

#endif // QVM_EXECUTION_H
