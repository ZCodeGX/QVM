#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/qvm.h"

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

static uint8_t *build_qcb(uint32_t *out_size) {
    uint8_t *d = (uint8_t *)calloc(1024, 1);
    if (!d) return NULL;
    uint32_t o = 0;

    w32(d + o, QCB_MAGIC);       o += 4;
    w32(d + o, QCB_VERSION_1_0); o += 4;
    w32(d + o, 0);             o += 4;
    w32(d + o, 2);             o += 4;

    w32(d + o, QCB_SECTION_CONSTANTS); o += 4;
    w32(d + o, 2 * (uint32_t)sizeof(qvm_value)); o += 4;
    qvm_value c0 = qvm_value_make(QVM_TYPE_I32, 2);
    qvm_value c1 = qvm_value_make(QVM_TYPE_I32, 3);
    memcpy(d + o, &c0, sizeof c0); o += (uint32_t)sizeof c0;
    memcpy(d + o, &c1, sizeof c1); o += (uint32_t)sizeof c1;

    w32(d + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;

    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 1;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 1); o += 4;

    d[o++] = QVM_OP_ARITH_ADD; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 0; d[o++] = 1;

    d[o++] = QVM_OP_RETURN; d[o++] = 0xFF;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 2;

    w32(d + size_at, o - start);
    *out_size = o;
    return d;
}

int main(void) {
    printf("=== QVM Demo End-to-End ===\n\n");

    uint32_t qcb_size = 0;
    uint8_t *qcb = build_qcb(&qcb_size);
    if (!qcb) { printf("ERROR: build QCB\n"); return 1; }
    printf("[1] QCB construido en memoria (%u bytes)\n", qcb_size);

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    if (!ctx) { printf("ERROR: contexto: %s\n", qvm_error_message(err)); free(qcb); return 1; }
    printf("[2] Contexto QVM creado\n");

    qvm_module *mod = qvm_module_load_from_memory(qcb, qcb_size, ctx, &err);
    free(qcb);
    if (!mod) { printf("ERROR: loader: %s\n", qvm_error_message(err)); qvm_context_destroy(ctx); return 1; }
    printf("[3] Modulo cargado (%u constantes, %u bytes de bytecode)\n",
           mod->constant_count, mod->bytecode_size);

    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);
    if (!rt) { printf("ERROR: runtime: %s\n", qvm_error_message(err)); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }
    printf("[4] Runtime creado\n");

    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *frame = qvm_frame_create(0, 3, types, NULL, &err);
    if (!frame) { printf("ERROR: frame: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    printf("[5] Frame creado con 3 registros\n\n");

    printf("Ejecutando bytecode: 2 + 3\n");
    err = qvm_runtime_execute(rt);
    if (err != QVM_OK) {
        printf("ERROR: ejecucion: %s\n", qvm_error_message(err));
        qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx);
        return 1;
    }

    qvm_value v;
    qvm_register_read(frame, 2, &v);
    printf("\n[6] registro r2 = %lld\n", (long long)qvm_value_as_i32(&v));
    printf("[7] retorno   = %lld\n", (long long)qvm_value_as_i32(&rt->return_value));
    printf("\n=== Demo completada ===\n");

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
    return 0;
}
