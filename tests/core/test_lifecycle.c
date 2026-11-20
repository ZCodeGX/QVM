#include <assert.h>
#include <stdio.h>
#include "qvm/qvm.h"

static void test_full_cycle(void) {
    qvm_error err = QVM_ERR_INTERNAL;
    qvm_context *ctx = qvm_context_create(NULL, &err);

    assert(ctx != NULL && err == QVM_OK);
    assert(qvm_state_of(ctx) == QVM_STATE_CREATED);

    assert(qvm_init(ctx) == QVM_OK);
    assert(qvm_state_of(ctx) == QVM_STATE_READY);

    assert(qvm_start(ctx) == QVM_OK);
    assert(qvm_state_of(ctx) == QVM_STATE_RUNNING);

    assert(qvm_stop(ctx) == QVM_OK);
    assert(qvm_state_of(ctx) == QVM_STATE_STOPPED);

    assert(qvm_reset(ctx) == QVM_OK);
    assert(qvm_state_of(ctx) == QVM_STATE_CREATED);

    qvm_context_destroy(ctx);
}

static void test_invalid_transitions(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);

    assert(qvm_start(ctx) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_stop(ctx) == QVM_ERR_INVALID_ARGUMENT);

    qvm_init(ctx);

    assert(qvm_init(ctx) == QVM_ERR_INVALID_ARGUMENT);

    qvm_start(ctx);

    assert(qvm_init(ctx) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_start(ctx) == QVM_ERR_INVALID_ARGUMENT);

    qvm_context_destroy(ctx);
}

static void test_null_context(void) {
    assert(qvm_init(NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_start(NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_stop(NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_reset(NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_state_of(NULL) == QVM_STATE_FAILED);
}

int main(void) {
    test_full_cycle();
    test_invalid_transitions();
    test_null_context();

    printf("test_lifecycle: OK\n");
    return 0;
}
