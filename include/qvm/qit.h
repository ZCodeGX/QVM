#ifndef QVM_QIT_H
#define QVM_QIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct qit_code qit_code;

// Firma canónica de una función compilada por QIT
typedef int64_t (*qit_func_t)(void *frame);

void qit_set_debug(int enabled);
qit_code *qit_compile(const uint8_t *bytecode, uint32_t size);
void qit_destroy(qit_code *code);
void *qit_get_entry(qit_code *code);
size_t qit_get_size(qit_code *code);

#ifdef __cplusplus
}
#endif

#endif // QVM_QIT_H
