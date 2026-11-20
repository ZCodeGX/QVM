#ifndef QVM_GCQ_H
#define QVM_GCQ_H

#include <stddef.h>
#include <stdint.h>
#include "qvm/error.h"
#include "qvm/limits.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gcq_context   gcq_context;
typedef struct gcq_allocator gcq_allocator;
typedef struct gcq_region    gcq_region;

gcq_context   *gcq_context_create(const qvm_limits *limits, qvm_error *err);
void           gcq_context_destroy(gcq_context *gcq);
uint64_t       gcq_context_bytes_allocated(const gcq_context *gcq);
uint64_t       gcq_context_bytes_limit(const gcq_context *gcq);

gcq_allocator *gcq_allocator_create(gcq_context *gcq, const char *strategy, qvm_error *err);
void           gcq_allocator_destroy(gcq_allocator *alloc);

void *gcq_alloc(gcq_allocator *alloc, size_t size, size_t align);
void  gcq_free(gcq_allocator *alloc, void *ptr);

gcq_region *gcq_region_create(gcq_allocator *alloc, qvm_error *err);
void        gcq_region_destroy(gcq_region *region);
void       *gcq_region_alloc(gcq_region *region, size_t size, size_t align);
void        gcq_region_release(gcq_region *region);

#ifdef __cplusplus
}
#endif

#endif
