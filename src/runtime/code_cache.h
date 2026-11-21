#ifndef QVM_CODE_CACHE_H
#define QVM_CODE_CACHE_H

#include <stdint.h>
#include "qvm/backend.h"
#include "qvm/loader.h"

/* Clave única para entrada de caché */
typedef struct {
    uint32_t module_id;
    uint32_t function_index;
    uint32_t qcb_version;
    qvm_backend_id_t backend_id;
    uint32_t flags;
} qvm_cache_key_t;

/* Entrada de caché */
typedef struct qvm_cache_entry {
    qvm_cache_key_t key;
    const qvm_compiled_code *code;
    struct qvm_cache_entry *next;  /* Para colisión hash */
} qvm_cache_entry_t;

/* Caché de código compilado */
typedef struct qvm_code_cache_t {
    qvm_cache_entry_t **buckets;
    size_t bucket_count;
    size_t entry_count;
    size_t max_entries;
} qvm_code_cache_t;

/* API de caché */
qvm_code_cache_t *qvm_code_cache_create(size_t max_entries);
void qvm_code_cache_destroy(qvm_code_cache_t *cache);

const qvm_compiled_code *qvm_code_cache_get(
    qvm_code_cache_t *cache,
    const qvm_cache_key_t *key);

int qvm_code_cache_put(
    qvm_code_cache_t *cache,
    const qvm_cache_key_t *key,
    const qvm_compiled_code *code);

void qvm_code_cache_invalidate(
    qvm_code_cache_t *cache,
    uint32_t module_id);

#endif // QVM_CODE_CACHE_H
