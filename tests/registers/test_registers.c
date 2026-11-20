#include <assert.h>
#include <stdio.h>
#include "qvm/qvm.h"

static qvm_frame *make_frame(void) {
    qvm_type_id_t types[6] = {
        QVM_TYPE_I32,
        QVM_TYPE_I64,
        QVM_TYPE_I128,
        QVM_TYPE_I128,
        QVM_TYPE_BOOL,
        QVM_TYPE_VOID
    };

    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 6, types, NULL, &err);
    assert(f != NULL && err == QVM_OK);
    return f;
}

static void test_read_write(void) {
    qvm_frame *f = make_frame();
    qvm_value v = {0};

    assert(qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 42)) == QVM_OK);
    assert(qvm_register_read(f, 0, &v) == QVM_OK);
    assert(v.type_id == QVM_TYPE_I32 && v.payload == 42);

    qvm_frame_destroy(f);
}

static void test_out_of_bounds(void) {
    qvm_frame *f = make_frame();
    qvm_value v = {0};

    assert(qvm_register_read(f, 6, &v) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_register_write(f, 6, qvm_value_make(QVM_TYPE_I32, 0)) == QVM_ERR_INVALID_ARGUMENT);

    qvm_frame_destroy(f);
}

static void test_void_register(void) {
    qvm_frame *f = make_frame();
    qvm_value v = {0};

    assert(qvm_register_read(f, 5, &v) == QVM_ERR_TYPE_MISMATCH);
    assert(qvm_register_write(f, 5, qvm_value_make(QVM_TYPE_VOID, 0)) == QVM_ERR_TYPE_MISMATCH);

    qvm_frame_destroy(f);
}

static void test_type_mismatch(void) {
    qvm_frame *f = make_frame();

    assert(qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I64, 0)) == QVM_ERR_TYPE_MISMATCH);

    qvm_frame_destroy(f);
}

static void test_pair(void) {
    qvm_frame *f = make_frame();
    uint64_t lo = 0;
    uint64_t hi = 0;

    assert(qvm_register_write_pair(f, 2, 0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL) == QVM_OK);
    assert(qvm_register_read_pair(f, 2, &lo, &hi) == QVM_OK);
    assert(lo == 0x1122334455667788ULL && hi == 0x99AABBCCDDEEFF00ULL);

    assert(qvm_register_write_pair(f, 1, 0, 0) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_register_read_pair(f, 1, &lo, &hi) == QVM_ERR_INVALID_ARGUMENT);

    assert(qvm_register_write_pair(f, 6, 0, 0) == QVM_ERR_INVALID_ARGUMENT);

    assert(qvm_register_write_pair(f, 4, 0, 0) == QVM_ERR_TYPE_MISMATCH);
    assert(qvm_register_read_pair(f, 4, &lo, &hi) == QVM_ERR_TYPE_MISMATCH);

    qvm_frame_destroy(f);
}

static void test_privacy(void) {
    qvm_type_id_t types[1] = { QVM_TYPE_I32 };
    qvm_error err;

    qvm_frame *parent = qvm_frame_create(0, 1, types, NULL, &err);
    qvm_frame *child = qvm_frame_create(1, 1, types, parent, &err);
    assert(parent != NULL && child != NULL);

    assert(qvm_register_write(parent, 0, qvm_value_make(QVM_TYPE_I32, 100)) == QVM_OK);
    assert(qvm_register_write(child, 0, qvm_value_make(QVM_TYPE_I32, 999)) == QVM_OK);

    qvm_value pv = {0};
    qvm_value cv = {0};

    qvm_register_read(parent, 0, &pv);
    qvm_register_read(child, 0, &cv);

    assert(pv.payload == 100 && cv.payload == 999);

    qvm_frame_destroy(child);
    qvm_frame_destroy(parent);
}

int main(void) {
    test_read_write();
    test_out_of_bounds();
    test_void_register();
    test_type_mismatch();
    test_pair();
    test_privacy();

    printf("test_registers: OK\n");
    return 0;
}
