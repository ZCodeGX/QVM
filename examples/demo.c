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
    w32(d + o, 3);             o += 4;

    // Sección de constantes
    w32(d + o, QCB_SECTION_CONSTANTS); o += 4;
    w32(d + o, 2 * (uint32_t)sizeof(qvm_value)); o += 4;
    qvm_value c0 = qvm_value_make(QVM_TYPE_I32, 2);
    qvm_value c1 = qvm_value_make(QVM_TYPE_I32, 3);
    memcpy(d + o, &c0, sizeof c0); o += (uint32_t)sizeof c0;
    memcpy(d + o, &c1, sizeof c1); o += (uint32_t)sizeof c1;

    // Sección de funciones
    w32(d + o, QCB_SECTION_FUNCTIONS); o += 4;
    uint32_t func_size_at = o; o += 4;
    uint32_t func_start = o;

    // Función 0: add(a: i32, b: i32) -> i32
    // Formato: bytecode_offset(4) + param_count(2) + local_count(2) + return_type(2) + local_types[local_count](2 c/u)
    w32(d + o, 0);  o += 4;  // bytecode_offset (se actualizará después)
    w16(d + o, 2);  o += 2;  // param_count
    w16(d + o, 3);  o += 2;  // local_count (2 params + 1 resultado)
    w16(d + o, QVM_TYPE_I32); o += 2;  // return_type
    w16(d + o, QVM_TYPE_I32); o += 2;  // local_types[0] = param a
    w16(d + o, QVM_TYPE_I32); o += 2;  // local_types[1] = param b
    w16(d + o, QVM_TYPE_I32); o += 2;  // local_types[2] = resultado

    // Función 1: main() -> i32
    w32(d + o, 0);  o += 4;  // bytecode_offset (inicio)
    w16(d + o, 0);  o += 2;  // param_count
    w16(d + o, 3);  o += 2;  // local_count
    w16(d + o, QVM_TYPE_I32); o += 2;  // return_type
    w16(d + o, QVM_TYPE_I32); o += 2;  // local_types[0]
    w16(d + o, QVM_TYPE_I32); o += 2;  // local_types[1]
    w16(d + o, QVM_TYPE_I32); o += 2;  // local_types[2]

    w32(d + func_size_at, o - func_start);

    // Sección de bytecode
    w32(d + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    // main (función 1): bytecode empieza en offset 0
    // LOAD_CONST r0, const 0
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 0;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;  // constante 0 (valor 2)

    // LOAD_CONST r1, const 1
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 1;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 1); o += 4;  // constante 1 (valor 3)

    // CALL r2, func 0 (add), args [r0, r1]
    d[o++] = QVM_OP_CALL; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    w32(d + o, 0); o += 4;  // function_index 0 (add)
    d[o++] = 2;  // arg_count
    d[o++] = 0;  // arg 0 = r0
    d[o++] = 1;  // arg 1 = r1

    // RETURN r2
    d[o++] = QVM_OP_RETURN; d[o++] = 0xFF;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 2;

    // add (función 0): bytecode empieza aquí
    uint32_t add_offset = o - start;
    d[o++] = QVM_OP_ARITH_ADD; d[o++] = 2;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 0; d[o++] = 1;

    d[o++] = QVM_OP_RETURN; d[o++] = 0xFF;
    w16(d + o, QVM_TYPE_I32); o += 2;
    d[o++] = 2;

    w32(d + size_at, o - start);

    // Actualizar bytecode_offset de add
    uint32_t func0_offset = func_start + 0;  // primera función
    w32(d + func0_offset, add_offset);

    *out_size = o;
    return d;
}

int main(void) {
    printf("=== QVM Demo: Llamada a Función ===\n\n");

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
    printf("[3] Modulo cargado (%u constantes, %u funciones)\n",
           mod->constant_count, mod->function_count);

    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);
    if (!rt) { printf("ERROR: runtime: %s\n", qvm_error_message(err)); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }
    printf("[4] Runtime creado\n");

    // Crear frame para main (función 1)
    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *frame = qvm_frame_create(1, 3, types, NULL, &err);
    if (!frame) { printf("ERROR: frame: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    rt->pc            = 0;  // main empieza en offset 0
    printf("[5] Frame de main creado\n\n");

    printf("Ejecutando: main() que llama a add(2, 3)\n");
    err = qvm_runtime_execute(rt);
    if (err != QVM_OK) {
        printf("ERROR: ejecucion: %s\n", qvm_error_message(err));
        qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx);
        return 1;
    }

    printf("\n[6] Resultado de add(2, 3) = %lld\n", (long long)qvm_value_as_i32(&rt->return_value));
    printf("\n=== Demo completada ===\n");

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
    return 0;
}
