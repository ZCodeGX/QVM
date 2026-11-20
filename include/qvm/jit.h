#ifndef QVM_JIT_H
#define QVM_JIT_H

#include <stdint.h>
#include <stddef.h>

typedef struct qvm_jit_code {
    void *code;           // Puntero al código ejecutable
    size_t size;          // Tamaño del código
    uint32_t entry_point; // Punto de entrada
} qvm_jit_code;

qvm_jit_code *qvm_jit_compile(const uint8_t *bytecode, uint32_t size);
void qvm_jit_destroy(qvm_jit_code *jit);

#endif
