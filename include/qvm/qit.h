#ifndef QVM_QIT_H
#define QVM_QIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "qvm/context.h"
#include "qvm/error.h"
#include "qvm/loader.h"

/* Contexto de ejecución canónico (ABI única) */
typedef struct {
    const qvm_value *registers;
    uint32_t register_count;
    const uint8_t *bytecode;
    uint32_t bytecode_size;
    const qvm_type_id_t *types;
    void *os_env;
} qvm_exec_context;

/* Resultado estandarizado */
typedef struct {
    qvm_error error;
    int64_t payload;
    qvm_type_id_t type_id;
    uint8_t flags;
} qvm_exec_result;

/* Firma canónica de punto de entrada */
typedef qvm_exec_result (*qvm_entry_fn)(const qvm_exec_context *ctx);

/* Tipos opacos */
typedef struct qvm_qit qvm_qit;
typedef struct qvm_compiled_code qvm_compiled_code;

/* API pública */
qvm_qit *qvm_qit_create(const qvm_context *ctx, qvm_error *err);
const qvm_compiled_code *qvm_qit_compile(qvm_qit *qit, 
                                          const qvm_module *module, 
                                          uint32_t function_index, 
                                          qvm_error *err);
qvm_entry_fn qvm_qit_entry(const qvm_compiled_code *code);
size_t qvm_qit_size(const qvm_compiled_code *code);
const char *qvm_qit_backend(const qvm_compiled_code *code);
void qvm_qit_release(const qvm_compiled_code *code);
void qvm_qit_destroy(qvm_qit *qit);
void qvm_qit_set_debug(qvm_qit *qit, int enabled);

/* Wrappers deprecated (compatibilidad temporal) */
typedef struct qit_code qit_code;
typedef int64_t (*qit_func_t)(void *frame);

qit_code *qit_compile(const uint8_t *bytecode, uint32_t size);
void qit_destroy(qit_code *code);
void *qit_get_entry(qit_code *code);
size_t qit_get_size(qit_code *code);
void qit_set_debug(int enabled);

#ifdef __cplusplus
}
#endif

#endif // QVM_QIT_H
