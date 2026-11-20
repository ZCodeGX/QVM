#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/loader.h"
#include "qvm/runtime.h"

static void w32(uint8_t *b, uint32_t v) {
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
    b[2] = (uint8_t)((v >> 16) & 0xFF);
    b[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void w16(uint8_t *b, uint16_t v) {
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void emit_header(uint8_t *d, uint32_t *o, uint32_t section_count) {
    w32(d + *o, QCB_MAGIC);       *o += 4;
    w32(d + *o, QCB_VERSION_1_0); *o += 4;
    w32(d + *o, 0);             *o += 4;
    w32(d + *o, section_count); *o += 4;
}

static void emit_const_i32(uint8_t *d, uint32_t *o, int32_t val) {
    w16(d + *o, QVM_TYPE_I32); *o += 2;
    uint64_t payload = (uint64_t)(int64_t)val;
    memcpy(d + *o, &payload, 8); *o += 8;
}

static void emit_constants_section(uint8_t *d, uint32_t *o, uint32_t content_size) {
    w32(d + *o, QCB_SECTION_CONSTANTS); *o += 4;
    w32(d + *o, content_size); *o += 4;
}

static void emit_function(uint8_t *d, uint32_t *o, uint32_t bc_offset,
                          uint16_t param_count, uint16_t local_count,
                          qvm_type_id_t return_type,
                          const qvm_type_id_t *local_types) {
    w32(d + *o, bc_offset); *o += 4;
    w16(d + *o, param_count); *o += 2;
    w16(d + *o, local_count); *o += 2;
    w16(d + *o, return_type); *o += 2;
    for (uint16_t i = 0; i < local_count; ++i) {
        w16(d + *o, local_types[i]); *o += 2;
    }
}

static void emit_bytecode(uint8_t *d, uint32_t *o, const uint8_t *bc, uint32_t bc_size) {
    w32(d + *o, QCB_SECTION_BYTECODE); *o += 4;
    w32(d + *o, bc_size); *o += 4;
    memcpy(d + *o, bc, bc_size);
    *o += bc_size;
}

static qvm_runtime *load_and_run(uint8_t *qcb, uint32_t qcb_size,
                                 uint32_t func_index, uint32_t local_count,
                                 const qvm_type_id_t *local_types,
                                 qvm_error *out_err) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);

    qvm_module *mod = qvm_module_load_from_memory(qcb, qcb_size, ctx, &err);
    if (!mod) {
        qvm_context_destroy(ctx);
        *out_err = err;
        return NULL;
    }

    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);
    if (!rt) {
        qvm_module_destroy(mod);
        qvm_context_destroy(ctx);
        *out_err = err;
        return NULL;
    }

    qvm_frame *frame = qvm_frame_create(func_index, local_count, local_types, NULL, &err);
    if (!frame) {
        qvm_runtime_destroy(rt);
        qvm_module_destroy(mod);
        qvm_context_destroy(ctx);
        *out_err = err;
        return NULL;
    }

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    rt->pc            = 0;

    *out_err = QVM_OK;
    return rt;
}

static void test_call_simple(void) {
    uint8_t d[1024];
    uint32_t o = 0;

    emit_header(d, &o, 2);

    emit_constants_section(d, &o, 10);
    emit_const_i32(d, &o, 42);

    uint8_t bc[16];
    uint32_t bo = 0;
    bc[bo++] = QVM_OP_LOAD_CONST; bc[bo++] = 0;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    w32(bc + bo, 0); bo += 4;
    bc[bo++] = QVM_OP_RETURN; bc[bo++] = 0xFF;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    bc[bo++] = 0;

    emit_bytecode(d, &o, bc, bo);

    qvm_type_id_t rt_types[1] = { QVM_TYPE_I32 };
    qvm_error err;
    qvm_runtime *rt = load_and_run(d, o, 0, 1, rt_types, &err);
    assert(rt != NULL);

    err = qvm_runtime_execute(rt);
    assert(err == QVM_OK);
    assert(rt->has_return == 1);
    assert(qvm_value_as_i32(&rt->return_value) == 42);

    qvm_runtime_destroy(rt);
}

static void test_call_with_args(void) {
    uint8_t d[1024];
    uint32_t o = 0;

    emit_header(d, &o, 1);

    uint8_t bc[16];
    uint32_t bo = 0;
    bc[bo++] = QVM_OP_ARITH_ADD; bc[bo++] = 2;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    bc[bo++] = 0; bc[bo++] = 1;
    bc[bo++] = QVM_OP_RETURN; bc[bo++] = 0xFF;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    bc[bo++] = 2;

    emit_bytecode(d, &o, bc, bo);

    qvm_type_id_t rt_types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_error err;
    qvm_runtime *rt = load_and_run(d, o, 0, 3, rt_types, &err);
    assert(rt != NULL);

    qvm_frame *frame = rt->call_stack[0];
    qvm_register_write(frame, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 20));

    err = qvm_runtime_execute(rt);
    assert(err == QVM_OK);
    assert(rt->has_return == 1);
    assert(qvm_value_as_i32(&rt->return_value) == 30);

    qvm_runtime_destroy(rt);
}

static void test_nested_call(void) {
    uint8_t d[1024];
    uint32_t o = 0;

    emit_header(d, &o, 3);

    emit_constants_section(d, &o, 20);
    emit_const_i32(d, &o, 5);
    emit_const_i32(d, &o, 7);

    uint32_t func_size_at = o;
    w32(d + o, QCB_SECTION_FUNCTIONS); o += 4;
    w32(d + o, 0); o += 4;
    uint32_t func_start = o;

    qvm_type_id_t types0[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    emit_function(d, &o, 0, 2, 3, QVM_TYPE_I32, types0);

    qvm_type_id_t types1[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    emit_function(d, &o, 0, 0, 3, QVM_TYPE_I32, types1);

    w32(d + func_size_at + 4, o - func_start);

    uint8_t bc[64];
    uint32_t bo = 0;

    bc[bo++] = QVM_OP_LOAD_CONST; bc[bo++] = 0;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    w32(bc + bo, 0); bo += 4;

    bc[bo++] = QVM_OP_LOAD_CONST; bc[bo++] = 1;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    w32(bc + bo, 1); bo += 4;

    bc[bo++] = QVM_OP_CALL; bc[bo++] = 2;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    w32(bc + bo, 0); bo += 4;
    bc[bo++] = 2; bc[bo++] = 0; bc[bo++] = 1;

    bc[bo++] = QVM_OP_RETURN; bc[bo++] = 0xFF;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    bc[bo++] = 2;

    uint32_t add_offset = bo;
    bc[bo++] = QVM_OP_ARITH_ADD; bc[bo++] = 2;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    bc[bo++] = 0; bc[bo++] = 1;

    bc[bo++] = QVM_OP_RETURN; bc[bo++] = 0xFF;
    w16(bc + bo, QVM_TYPE_I32); bo += 2;
    bc[bo++] = 2;

    emit_bytecode(d, &o, bc, bo);

    uint32_t func0_offset = func_start;
    w32(d + func0_offset, add_offset);

    qvm_type_id_t rt_types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_error err;
    qvm_runtime *rt = load_and_run(d, o, 1, 3, rt_types, &err);
    assert(rt != NULL);

    err = qvm_runtime_execute(rt);
    assert(err == QVM_OK);
    assert(rt->has_return == 1);
    assert(qvm_value_as_i32(&rt->return_value) == 12);

    qvm_runtime_destroy(rt);
}

static void test_return_void(void) {
    uint8_t d[1024];
    uint32_t o = 0;

    emit_header(d, &o, 1);

    uint8_t bc[4];
    uint32_t bo = 0;
    bc[bo++] = QVM_OP_RETURN_VOID;
    bc[bo++] = 0; bc[bo++] = 0; bc[bo++] = 0;

    emit_bytecode(d, &o, bc, bo);

    qvm_type_id_t rt_types[1] = { QVM_TYPE_I32 };
    qvm_error err;
    qvm_runtime *rt = load_and_run(d, o, 0, 1, rt_types, &err);
    assert(rt != NULL);

    err = qvm_runtime_execute(rt);
    assert(err == QVM_OK);
    assert(rt->has_return == 0);

    qvm_runtime_destroy(rt);
}

static void test_call_depth_exceeded(void) {
    uint8_t d[1024];
    uint32_t o = 0;

    emit_header(d, &o, 2);

    uint32_t func_size_at = o;
    w32(d + o, QCB_SECTION_FUNCTIONS); o += 4;
    w32(d + o, 0); o += 4;
    uint32_t func_start = o;

    qvm_type_id_t types0[1] = { QVM_TYPE_I32 };
    emit_function(d, &o, 0, 0, 1, QVM_TYPE_VOID, types0);

    w32(d + func_size_at + 4, o - func_start);

    uint8_t bc[16];
    uint32_t bo = 0;
    // CALL con dst=0 (registro válido) en lugar de 0xFF
    bc[bo++] = QVM_OP_CALL; bc[bo++] = 0;
    w16(bc + bo, QVM_TYPE_VOID); bo += 2;
    w32(bc + bo, 0); bo += 4;
    bc[bo++] = 0;

    emit_bytecode(d, &o, bc, bo);

    qvm_config cfg = qvm_config_default();
    cfg.verify_on_load = 0;  // Deshabilitar verificación para este test
    cfg.limits.max_call_depth = 2;
    qvm_error err;
    qvm_context *ctx = qvm_context_create(&cfg, &err);
    assert(ctx != NULL);

    qvm_module *mod = qvm_module_load_from_memory(d, o, ctx, &err);
    assert(mod != NULL);

    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);
    assert(rt != NULL);

    qvm_type_id_t rt_types[1] = { QVM_TYPE_I32 };
    qvm_frame *frame = qvm_frame_create(0, 1, rt_types, NULL, &err);
    assert(frame != NULL);

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    rt->pc            = 0;

    err = qvm_runtime_execute(rt);
    assert(err == QVM_ERR_CALL_DEPTH_EXCEEDED);

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
}

int main(void) {
    test_call_simple();
    test_call_with_args();
    test_nested_call();
    test_return_void();
    test_call_depth_exceeded();
    printf("test_call_return: OK\n");
    return 0;
}
