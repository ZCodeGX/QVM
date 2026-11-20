#include <stdlib.h>
#include <string.h>
#include "qvm/gcq.h"

struct gcq_context {
    uint64_t bytes_allocated;
    uint64_t bytes_limit;
};

struct gcq_allocator {
    gcq_context *ctx;
    char         strategy[32];
};

struct gcq_region {
    gcq_allocator *alloc;
    void         **blocks;
    size_t         block_count;
    size_t         block_capacity;
};

gcq_context *gcq_context_create(const qvm_limits *limits, qvm_error *err) {
    if (err) *err = QVM_OK;
    gcq_context *gcq = (gcq_context *)calloc(1, sizeof *gcq);
    if (!gcq) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    gcq->bytes_limit = limits ? limits->max_memory_bytes : 67108864;
    return gcq;
}

void gcq_context_destroy(gcq_context *gcq) { free(gcq); }

uint64_t gcq_context_bytes_allocated(const gcq_context *gcq) {
    return gcq ? gcq->bytes_allocated : 0;
}

uint64_t gcq_context_bytes_limit(const gcq_context *gcq) {
    return gcq ? gcq->bytes_limit : 0;
}

gcq_allocator *gcq_allocator_create(gcq_context *gcq, const char *strategy, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!gcq || !strategy) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    gcq_allocator *alloc = (gcq_allocator *)calloc(1, sizeof *alloc);
    if (!alloc) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    alloc->ctx = gcq;
    strncpy(alloc->strategy, strategy, sizeof(alloc->strategy) - 1);
    return alloc;
}

void gcq_allocator_destroy(gcq_allocator *alloc) { free(alloc); }

void *gcq_alloc(gcq_allocator *alloc, size_t size, size_t align) {
    if (!alloc || size == 0) return NULL;
    if (alloc->ctx->bytes_allocated + size > alloc->ctx->bytes_limit) return NULL;
    void *ptr = NULL;
    if (align <= 8) {
        ptr = malloc(size);
    } else {
        size_t aligned_size = (size + align - 1) & ~(align - 1);
        ptr = aligned_alloc(align, aligned_size);
    }
    if (ptr) alloc->ctx->bytes_allocated += size;
    return ptr;
}

void gcq_free(gcq_allocator *alloc, void *ptr) {
    (void)alloc;
    free(ptr);
}

gcq_region *gcq_region_create(gcq_allocator *alloc, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!alloc) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    gcq_region *region = (gcq_region *)calloc(1, sizeof *region);
    if (!region) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    region->alloc          = alloc;
    region->block_capacity = 16;
    region->blocks         = (void **)malloc(region->block_capacity * sizeof(void *));
    if (!region->blocks) {
        free(region);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    return region;
}

void gcq_region_destroy(gcq_region *region) {
    if (!region) return;
    gcq_region_release(region);
    free(region->blocks);
    free(region);
}

void *gcq_region_alloc(gcq_region *region, size_t size, size_t align) {
    if (!region || size == 0) return NULL;
    void *ptr = gcq_alloc(region->alloc, size, align);
    if (!ptr) return NULL;
    if (region->block_count >= region->block_capacity) {
        size_t new_cap = region->block_capacity * 2;
        void **new_blocks = (void **)realloc(region->blocks, new_cap * sizeof(void *));
        if (!new_blocks) {
            gcq_free(region->alloc, ptr);
            return NULL;
        }
        region->blocks         = new_blocks;
        region->block_capacity = new_cap;
    }
    region->blocks[region->block_count++] = ptr;
    return ptr;
}

void gcq_region_release(gcq_region *region) {
    if (!region) return;
    for (size_t i = 0; i < region->block_count; ++i) {
        gcq_free(region->alloc, region->blocks[i]);
    }
    region->block_count = 0;
}
