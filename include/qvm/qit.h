#ifndef QVM_QIT_H
#define QVM_QIT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * QIT - QVM Inline Translation
 * 
 * Sistema de compilación que traduce bytecode QVM a código nativo x86-64.
 * Ofrece rendimiento cercano a C puro con la seguridad de la VM.
 */

typedef struct qit_code qit_code;

/**
 * Compila bytecode QVM a código nativo
 */
qit_code *qit_compile(const uint8_t *bytecode, uint32_t size);

/**
 * Libera el código compilado
 */
void qit_destroy(qit_code *code);

/**
 * Obtiene el puntero al código ejecutable
 */
void *qit_get_entry(qit_code *code);

/**
 * Obtiene el tamaño del código generado
 */
size_t qit_get_size(qit_code *code);

/**
 * Tipo de función compilada
 */
typedef int64_t (*qit_func_t)(void *frame);

/**
 * Habilita/deshabilita debug output
 */
void qit_set_debug(int enabled);

#ifdef __cplusplus
}
#endif

#endif /* QVM_QIT_H */
