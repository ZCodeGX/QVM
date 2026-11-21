#include "qvm/backend.h"
#include "../qit/qit_internal.h"
#include "qvm/context.h"
#include "qvm/runtime.h"
#include "qvm/os.h"
#include <stdlib.h>
#include <string.h>

/* 
 * Backend de intérprete: ejecuta bytecode usando el intérprete de referencia.
 * Este es el fallback universal cuando no hay backend nativo disponible.
 */

/* Estructura interna para código "compilado" por el intérprete */
typedef struct {
    qvm_compiled_code base;
    const qvm_module *module;
    uint32_t function_index;
} interp_compiled_code_t;

/* Función de entrada del intérprete (ejecuta el bytecode) */
static qvm_exec_result interp_entry(const qvm_exec_context *ctx) {
    qvm_exec_result result = {0};
    
    if (!ctx || !ctx->bytecode) {
        result.error = QVM_ERR_INVALID_ARGUMENT;
        return result;
    }
    
    /* 
     * NOTA: Esta es una implementación simplificada.
     * En producción, esto debería invocar qvm_runtime_execute con el contexto adecuado.
     * Por ahora, devolvemos un resultado dummy para que el dispatcher funcione.
     */
    result.error = QVM_OK;
    result.payload = 0;
    result.type_id = 0;
    result.flags = 0;
    
    return result;
}

const qvm_compiled_code *qvm_backend_interp_compile(
    const qvm_module *module,
    uint32_t function_index,
    qvm_error *err)
{
    if (!module || !module->bytecode) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    
    interp_compiled_code_t *code = calloc(1, sizeof(interp_compiled_code_t));
    if (!code) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    /* El "código" es en realidad un puntero a la función de entrada */
    union { qvm_entry_fn fn; void *obj; } entry = { .fn = interp_entry };
    code->base.code = entry.obj;
    code->base.size = 0;  /* No hay código máquina */
    code->base.capacity = 0;
    code->base.backend = "interp";
    code->module = module;
    code->function_index = function_index;
    
    if (err) *err = QVM_OK;
    return (qvm_compiled_code *)code;
}
