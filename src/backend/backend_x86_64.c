#include "qvm/backend.h"
#include "../qit/qit_internal.h"
#include "qvm/qit.h"
#include <stdlib.h>

/* 
 * Backend x86_64: usa el compilador QIT existente.
 * Este archivo actúa como puente entre la API de backends y QIT.
 */

const qvm_compiled_code *qvm_backend_x86_64_compile(
    const qvm_module *module,
    uint32_t function_index,
    qvm_error *err)
{
    (void)function_index;
    if (!module || !module->bytecode) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    
    /* Usar la API antigua de QIT (que ya está refactorizada) */
    qit_code *jit_code = qit_compile(module->bytecode, module->bytecode_size);
    if (!jit_code) {
        if (err) *err = QVM_ERR_INVALID_BYTECODE;
        return NULL;
    }
    
    /* Convertir qit_code a qvm_compiled_code */
    qvm_compiled_code *code = calloc(1, sizeof(qvm_compiled_code));
    if (!code) {
        qit_destroy(jit_code);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    code->code = jit_code->code;
    code->size = jit_code->size;
    code->capacity = jit_code->capacity;
    code->backend = "x86_64_sysv";
    
    /* Liberar el wrapper (no el código, que ahora es propiedad de code) */
    free(jit_code);
    
    if (err) *err = QVM_OK;
    return code;
}
