#include "code_cache.h"
#include <stdlib.h>
#include <string.h>

/* Hash simple para la clave */
static uint32_t hash_key(const qvm_cache_key_t *key) {
    uint32_t hash = key->module_id;
    hash = hash * 31 + key->function_index;
    hash = hash * 31 + key->qcb_version;
    hash = hash * 31 + key->backend_id;
    hash = hash * 31 + key->flags;
    return hash;
}

qvm_code_cache_t *qvm_code_cache_create(size_t max_entries) {
    qvm_code_cache_t *cache = calloc(1, sizeof(qvm_code_cache_t));
    if (!cache) return NULL;
    
    cache->bucket_count = 256;  /* Potencia de 2 para eficiencia */
    cache->buckets = calloc(cache->bucket_count, sizeof(qvm_cache_entry_t *));
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }
    
    cache->max_entries = max_entries;
    cache->entry_count = 0;
    return cache;
}

void qvm_code_cache_destroy(qvm_code_cache_t *cache) {
    if (!cache) return;
    
    /* Liberar todas las entradas */
    for (size_t i = 0; i < cache->bucket_count; i++) {
        qvm_cache_entry_t *entry = cache->buckets[i];
        while (entry) {
            qvm_cache_entry_t *next = entry->next;
            if (entry->code) {
                qvm_qit_release(entry->code);
            }
            free(entry);
            entry = next;
        }
    }
    
    free(cache->buckets);
    free(cache);
}

const qvm_compiled_code *qvm_code_cache_get(
    qvm_code_cache_t *cache,
    const qvm_cache_key_t *key)
{
    if (!cache || !key) return NULL;
    
    uint32_t hash = hash_key(key) % cache->bucket_count;
    qvm_cache_entry_t *entry = cache->buckets[hash];
    
    while (entry) {
        if (memcmp(&entry->key, key, sizeof(qvm_cache_key_t)) == 0) {
            return entry->code;
        }
        entry = entry->next;
    }
    
    return NULL;
}

int qvm_code_cache_put(
    qvm_code_cache_t *cache,
    const qvm_cache_key_t *key,
    const qvm_compiled_code *code)
{
    if (!cache || !key || !code) return 0;
    
    /* Verificar si ya existe */
    if (qvm_code_cache_get(cache, key)) {
        return 1;  /* Ya existe, no duplicar */
    }
    
    /* Verificar límite */
    if (cache->entry_count >= cache->max_entries) {
        return 0;  /* Caché llena */
    }
    
    /* Crear nueva entrada */
    qvm_cache_entry_t *entry = calloc(1, sizeof(qvm_cache_entry_t));
    if (!entry) return 0;
    
    entry->key = *key;
    entry->code = code;
    
    /* Insertar al principio de la lista */
    uint32_t hash = hash_key(key) % cache->bucket_count;
    entry->next = cache->buckets[hash];
    cache->buckets[hash] = entry;
    cache->entry_count++;
    
    return 1;
}

void qvm_code_cache_invalidate(
    qvm_code_cache_t *cache,
    uint32_t module_id)
{
    if (!cache) return;
    
    /* Invalidar todas las entradas del módulo */
    for (size_t i = 0; i < cache->bucket_count; i++) {
        qvm_cache_entry_t *entry = cache->buckets[i];
        qvm_cache_entry_t *prev = NULL;
        
        while (entry) {
            if (entry->key.module_id == module_id) {
                /* Eliminar entrada */
                if (prev) {
                    prev->next = entry->next;
                } else {
                    cache->buckets[i] = entry->next;
                }
                
                qvm_cache_entry_t *to_free = entry;
                entry = entry->next;
                
                if (to_free->code) {
                    qvm_qit_release(to_free->code);
                }
                free(to_free);
                cache->entry_count--;
            } else {
                prev = entry;
                entry = entry->next;
            }
        }
    }
}
