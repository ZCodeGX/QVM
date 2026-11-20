#include <assert.h>
#include <stdio.h>
#include "qvm/qvm.h"

static void test_create_destroy(void) {
    qvm_type_id_t types[4] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_F64, QVM_TYPE_BOOL };
    qvm_error err = QVM_ERR_INTERNAL;

    qvm_frame *f = qvm_frame_create(7, 4, types, NULL, &err);
    assert(f != NULL && err == QVM_OK);
    assert(qvm_frame_register_count(f) == 4);
    assert(qvm_frame_function_index(f) == 7);

    const qvm_type_id_t *ftypes = qvm_frame_register_types(f);
    const qvm_value *regs = qvm_frame_registers_const(f);

    for (uint32_t i = 0; i < 4; ++i) {
        assert(ftypes[i] == types[i]);
        assert(regs[i].type_id == types[i]);
        assert(regs[i].payload == 0);
    }

    qvm_frame_destroy(f);
}

static void test_invalid_args(void) {
    qvm_error err;
    qvm_type_id_t t[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };

    assert(qvm_frame_create(0, 0, t, NULL, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);

    assert(qvm_frame_create(0, 2, NULL, NULL, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
}

int main(void) {
    test_create_destroy();
    test_invalid_args();
    qvm_frame_destroy(NULL);

    printf("test_frames: OK\n");
    return 0;
}
