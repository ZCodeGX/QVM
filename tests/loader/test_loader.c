#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/loader.h"

static void w16(uint8_t *b, uint16_t v) {
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void w32(uint8_t *b, uint32_t v) {
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
    b[2] = (uint8_t)((v >> 16) & 0xFF);
    b[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void test_invalid_args(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    assert(qvm_module_load_from_file(NULL, ctx, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_module_load_from_file("test.qcb", NULL, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_module_load_from_memory(NULL, 100, ctx, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    qvm_context_destroy(ctx);
}

static void test_invalid_magic(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    uint8_t data[16] = {0};
    data[0] = 0xFF;
    qvm_module *mod = qvm_module_load_from_memory(data, 16, ctx, &err);
    assert(mod == NULL);
    assert(err == QVM_ERR_INVALID_BYTECODE);
    qvm_context_destroy(ctx);
}

static void test_invalid_version(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    uint8_t data[16] = {0};
    data[0] = 0x51; data[1] = 0x41; data[2] = 0x4D; data[3] = 0x52;
    data[4] = 0x02;
    qvm_module *mod = qvm_module_load_from_memory(data, 16, ctx, &err);
    assert(mod == NULL);
    assert(err == QVM_ERR_MODULE_INCOMPATIBLE);
    qvm_context_destroy(ctx);
}

static void test_file_not_found(void) {
    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    assert(ctx != NULL);
    qvm_module *mod = qvm_module_load_from_file("nonexistent.qcb", ctx, &err);
    assert(mod == NULL);
    assert(err == QVM_ERR_IO);
    qvm_context_destroy(ctx);
}

static void build_qcb_with_jump(uint8_t *buf, uint32_t *out_size) {
    uint32_t o = 0;
    w32(buf + o, QCB_MAGIC);       o += 4;
    w32(buf + o, QCB_VERSION_1_0); o += 4;
    w32(buf + o, 0);               o += 4;
    w32(buf + o, 1);               o += 4;

    w32(buf + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    buf[o++] = QVM_OP_JMP; buf[o++] = 0x00;
    w16(buf + o, QVM_TYPE_VOID); o += 2;
    w32(buf + o, 0xFFFFFFFF); o += 4;

    w32(buf + size_at, o - start);
    *out_size = o;
}

static void test_verify_on_load_rejects_bad_bytecode(void) {
    qvm_error err;
    qvm_config cfg = qvm_config_default();
    cfg.verify_on_load = 1;
    qvm_context *ctx = qvm_context_create(&cfg, &err);
    assert(ctx != NULL);

    uint8_t buf[256];
    uint32_t sz = 0;
    build_qcb_with_jump(buf, &sz);

    qvm_module *mod = qvm_module_load_from_memory(buf, sz, ctx, &err);
    assert(mod == NULL);
    // El verifier devuelve el error específico (INVALID_BYTECODE), no VERIFICATION_FAILED
    assert(err == QVM_ERR_INVALID_BYTECODE);

    qvm_context_destroy(ctx);
}

static void test_verify_on_load_disabled_accepts_bad_bytecode(void) {
    qvm_error err;
    qvm_config cfg = qvm_config_default();
    cfg.verify_on_load = 0;
    qvm_context *ctx = qvm_context_create(&cfg, &err);
    assert(ctx != NULL);

    uint8_t buf[256];
    uint32_t sz = 0;
    build_qcb_with_jump(buf, &sz);

    qvm_module *mod = qvm_module_load_from_memory(buf, sz, ctx, &err);
    assert(mod != NULL);
    assert(err == QVM_OK);

    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
}

int main(void) {
    test_invalid_args();
    test_invalid_magic();
    test_invalid_version();
    test_file_not_found();
    test_verify_on_load_rejects_bad_bytecode();
    test_verify_on_load_disabled_accepts_bad_bytecode();
    printf("test_loader: OK\n");
    return 0;
}
