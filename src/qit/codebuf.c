#include <stdlib.h>
#include <string.h>
#include "qvm/qit.h"

#define CODEBUF_INITIAL_CAPACITY 4096
#define CODEBUF_MAX_CAPACITY (64 * 1024 * 1024) // 64 MiB

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    int overflow;
} codebuf_t;

codebuf_t *codebuf_create(void) {
    codebuf_t *cb = calloc(1, sizeof(codebuf_t));
    if (!cb) return NULL;
    
    cb->data = malloc(CODEBUF_INITIAL_CAPACITY);
    if (!cb->data) {
        free(cb);
        return NULL;
    }
    
    cb->capacity = CODEBUF_INITIAL_CAPACITY;
    cb->size = 0;
    cb->overflow = 0;
    return cb;
}

void codebuf_destroy(codebuf_t *cb) {
    if (cb) {
        free(cb->data);
        free(cb);
    }
}

int codebuf_reserve(codebuf_t *cb, size_t additional) {
    if (cb->overflow) return 0;
    
    size_t required = cb->size + additional;
    if (required <= cb->capacity) return 1;
    
    size_t new_capacity = cb->capacity;
    while (new_capacity < required) {
        new_capacity *= 2;
        if (new_capacity > CODEBUF_MAX_CAPACITY) {
            cb->overflow = 1;
            return 0;
        }
    }
    
    uint8_t *new_data = realloc(cb->data, new_capacity);
    if (!new_data) {
        cb->overflow = 1;
        return 0;
    }
    
    cb->data = new_data;
    cb->capacity = new_capacity;
    return 1;
}

void codebuf_emit8(codebuf_t *cb, uint8_t value) {
    if (cb->overflow || !codebuf_reserve(cb, 1)) return;
    cb->data[cb->size++] = value;
}

void codebuf_emit16(codebuf_t *cb, uint16_t value) {
    if (cb->overflow || !codebuf_reserve(cb, 2)) return;
    cb->data[cb->size++] = (uint8_t)(value & 0xFF);
    cb->data[cb->size++] = (uint8_t)((value >> 8) & 0xFF);
}

void codebuf_emit32(codebuf_t *cb, uint32_t value) {
    if (cb->overflow || !codebuf_reserve(cb, 4)) return;
    cb->data[cb->size++] = (uint8_t)(value & 0xFF);
    cb->data[cb->size++] = (uint8_t)((value >> 8) & 0xFF);
    cb->data[cb->size++] = (uint8_t)((value >> 16) & 0xFF);
    cb->data[cb->size++] = (uint8_t)((value >> 24) & 0xFF);
}

void codebuf_emit64(codebuf_t *cb, uint64_t value) {
    if (cb->overflow || !codebuf_reserve(cb, 8)) return;
    for (int i = 0; i < 8; i++) {
        cb->data[cb->size++] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
}

size_t codebuf_tell(codebuf_t *cb) {
    return cb->size;
}

void codebuf_patch32(codebuf_t *cb, size_t offset, uint32_t value) {
    if (cb->overflow || offset + 4 > cb->size) return;
    cb->data[offset + 0] = (uint8_t)(value & 0xFF);
    cb->data[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
    cb->data[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
    cb->data[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
}

uint8_t *codebuf_data(codebuf_t *cb) {
    return cb->data;
}

size_t codebuf_size(codebuf_t *cb) {
    return cb->size;
}

int codebuf_has_overflow(codebuf_t *cb) {
    return cb->overflow;
}

void codebuf_seal(codebuf_t *cb) {
    // No-op por ahora, pero podría hacer shrink-to-fit
    (void)cb;
}
