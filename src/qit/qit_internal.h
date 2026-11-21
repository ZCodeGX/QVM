#ifndef QVM_QIT_INTERNAL_H
#define QVM_QIT_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include "qvm/qit.h"

/* Definición completa de qit_code (interna, no pública) */
struct qit_code {
    void *code;
    size_t size;
    size_t capacity;
};

/* Definición completa de qvm_compiled_code (interna, no pública) */
struct qvm_compiled_code {
    void *code;
    size_t size;
    size_t capacity;
    const char *backend;
};

#endif // QVM_QIT_INTERNAL_H
