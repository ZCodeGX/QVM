#include <assert.h>
#include <stdio.h>
#include "qvm/qvm.h"
#include "qvm/gcq.h"

static void test_context_create_destroy(void) {
    qvm_error err;
    qvm_limits limits = qvm_limits_default();
    gcq_context *gcq = gcq_context_create(&limits, &err);
    assert(gcq != NULL && err == QVM_OK);
    assert(gcq_context_bytes_allocated(gcq) == 0);
    assert(gcq_context_bytes_limit(gcq) == limits.max_memory_bytes);
    gcq_context_destroy(gcq);
}

static void test_allocator_bump(void) {
    qvm_error err;
    qvm_limits limits = qvm_limits_default();
    gcq_context *gcq = gcq_context_create(&limits, &err);
    assert(gcq != NULL);
    gcq_allocator *alloc = gcq_allocator_create(gcq, "bump", &err);
    assert(alloc != NULL && err == QVM_OK);
    void *ptr1 = gcq_alloc(alloc, 100, 8);
    assert(ptr1 != NULL);
    assert(gcq_context_bytes_allocated(gcq) == 100);
    void *ptr2 = gcq_alloc(alloc, 200, 8);
    assert(ptr2 != NULL);
    assert(gcq_context_bytes_allocated(gcq) == 300);
    gcq_free(alloc, ptr1);
    gcq_free(alloc, ptr2);
    gcq_allocator_destroy(alloc);
    gcq_context_destroy(gcq);
}

static void test_allocator_limit(void) {
    qvm_error err;
    qvm_limits limits = qvm_limits_default();
    limits.max_memory_bytes = 1000;
    gcq_context *gcq = gcq_context_create(&limits, &err);
    assert(gcq != NULL);
    gcq_allocator *alloc = gcq_allocator_create(gcq, "bump", &err);
    assert(alloc != NULL);
    void *ptr1 = gcq_alloc(alloc, 500, 8);
    assert(ptr1 != NULL);
    void *ptr2 = gcq_alloc(alloc, 600, 8);
    assert(ptr2 == NULL);
    gcq_free(alloc, ptr1);
    gcq_allocator_destroy(alloc);
    gcq_context_destroy(gcq);
}

static void test_region(void) {
    qvm_error err;
    qvm_limits limits = qvm_limits_default();
    gcq_context *gcq = gcq_context_create(&limits, &err);
    assert(gcq != NULL);
    gcq_allocator *alloc = gcq_allocator_create(gcq, "bump", &err);
    assert(alloc != NULL);
    gcq_region *region = gcq_region_create(alloc, &err);
    assert(region != NULL && err == QVM_OK);
    void *ptr1 = gcq_region_alloc(region, 100, 8);
    assert(ptr1 != NULL);
    assert(gcq_context_bytes_allocated(gcq) == 100);
    void *ptr2 = gcq_region_alloc(region, 200, 8);
    assert(ptr2 != NULL);
    assert(gcq_context_bytes_allocated(gcq) == 300);
    gcq_region_release(region);
    gcq_region_destroy(region);
    gcq_allocator_destroy(alloc);
    gcq_context_destroy(gcq);
}

static void test_region_multiple_blocks(void) {
    qvm_error err;
    qvm_limits limits = qvm_limits_default();
    gcq_context *gcq = gcq_context_create(&limits, &err);
    assert(gcq != NULL);
    gcq_allocator *alloc = gcq_allocator_create(gcq, "bump", &err);
    assert(alloc != NULL);
    gcq_region *region = gcq_region_create(alloc, &err);
    assert(region != NULL);
    for (int i = 0; i < 20; ++i) {
        void *ptr = gcq_region_alloc(region, 50, 8);
        assert(ptr != NULL);
    }
    assert(gcq_context_bytes_allocated(gcq) == 1000);
    gcq_region_destroy(region);
    gcq_allocator_destroy(alloc);
    gcq_context_destroy(gcq);
}

static void test_null_args(void) {
    qvm_error err;
    assert(gcq_context_create(NULL, &err) != NULL);
    assert(gcq_allocator_create(NULL, "bump", &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(gcq_region_create(NULL, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(gcq_alloc(NULL, 10, 8) == NULL);
    gcq_free(NULL, NULL);
    gcq_region_destroy(NULL);
    gcq_region_release(NULL);
}

int main(void) {
    test_context_create_destroy();
    test_allocator_bump();
    test_allocator_limit();
    test_region();
    test_region_multiple_blocks();
    test_null_args();
    printf("test_gcq: OK\n");
    return 0;
}
