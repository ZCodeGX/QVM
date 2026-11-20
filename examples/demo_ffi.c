#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/qvm.h"

// Función nativa: imprime un i32
static qvm_error native_print_i32(qvm_frame *frame, const qvm_value *args, 
                                   uint32_t arg_count, qvm_value *result) {
    (void)frame;
    (void)result;
    if (arg_count != 1) return QVM_ERR_INVALID_ARGUMENT;
    printf("  [FFI] print_i32(%lld)\n", (long long)args[0].payload);
    result->type_id = QVM_TYPE_VOID;
    return QVM_OK;
}

// Función nativa: retorna 42
static qvm_error native_get_answer(qvm_frame *frame, const qvm_value *args,
                                    uint32_t arg_count, qvm_value *result) {
    (void)frame;
    (void)args;
    if (arg_count != 0) return QVM_ERR_INVALID_ARGUMENT;
    result->type_id = QVM_TYPE_I32;
    result->payload = 42;
    return QVM_OK;
}

// Función nativa: suma dos i32
static qvm_error native_add(qvm_frame *frame, const qvm_value *args,
                             uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 2) return QVM_ERR_INVALID_ARGUMENT;
    result->type_id = QVM_TYPE_I32;
    result->payload = args[0].payload + args[1].payload;
    return QVM_OK;
}

static void write32(uint8_t *buf, uint32_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
    buf[2] = (uint8_t)((val >> 16) & 0xFF);
    buf[3] = (uint8_t)((val >> 24) & 0xFF);
}

static void write16(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
}

static uint8_t *build_ffi_qcb(uint32_t *out_size) {
    uint8_t *d = (uint8_t *)calloc(1024, 1);
    if (!d) return NULL;
    uint32_t o = 0;

    // Header QCB
    write32(d + o, QCB_MAGIC);       o += 4;
    write32(d + o, QCB_VERSION_1_0); o += 4;
    write32(d + o, 0);               o += 4;
    write32(d + o, 2);               o += 4; // 2 secciones

    // Sección 1: Constantes
    write32(d + o, QCB_SECTION_CONSTANTS); o += 4;
    write32(d + o, 2 * (uint32_t)sizeof(qvm_value)); o += 4;
    qvm_value c0 = qvm_value_make(QVM_TYPE_I32, 10);
    qvm_value c1 = qvm_value_make(QVM_TYPE_I32, 20);
    memcpy(d + o, &c0, sizeof c0); o += (uint32_t)sizeof c0;
    memcpy(d + o, &c1, sizeof c1); o += (uint32_t)sizeof c1;

    // Sección 2: Bytecode
    write32(d + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    // LOAD_CONST r0, const 0 (valor 10)
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 0;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 0); o += 4;

    // LOAD_CONST r1, const 1 (valor 20)
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 1;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 1); o += 4;

    // CALL_NATIVE r2, func 2 (add), args [r0, r1]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 2;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 2); o += 4; // function_index 2 (add)
    d[o++] = 2;  // arg_count
    d[o++] = 0;  // arg 0 = r0
    d[o++] = 1;  // arg 1 = r1

    // CALL_NATIVE r3, func 0 (print_i32), args [r2]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 3;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 0); o += 4; // function_index 0 (print_i32)
    d[o++] = 1;  // arg_count
    d[o++] = 2;  // arg 0 = r2

    // CALL_NATIVE r4, func 1 (get_answer), args []
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 4;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 1); o += 4; // function_index 1 (get_answer)
    d[o++] = 0;  // arg_count

    // CALL_NATIVE r5, func 0 (print_i32), args [r4]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 5;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 0); o += 4; // function_index 0 (print_i32)
    d[o++] = 1;  // arg_count
    d[o++] = 4;  // arg 0 = r4

    // RETURN_VOID
    d[o++] = QVM_OP_RETURN_VOID;
    d[o++] = 0; d[o++] = 0; d[o++] = 0;

    write32(d + size_at, o - start);
    *out_size = o;
    return d;
}

int main(void) {
    printf("=== QVM Demo: FFI (Foreign Function Interface) ===\n\n");

    uint32_t qcb_size = 0;
    uint8_t *qcb = build_ffi_qcb(&qcb_size);
    if (!qcb) { printf("ERROR: build QCB\n"); return 1; }
    printf("[1] QCB construido en memoria (%u bytes)\n", qcb_size);

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    if (!ctx) { printf("ERROR: contexto: %s\n", qvm_error_message(err)); free(qcb); return 1; }
    printf("[2] Contexto QVM creado\n");

    qvm_module *mod = qvm_module_load_from_memory(qcb, qcb_size, ctx, &err);
    free(qcb);
    if (!mod) { printf("ERROR: loader: %s\n", qvm_error_message(err)); qvm_context_destroy(ctx); return 1; }
    printf("[3] Modulo cargado (%u constantes)\n", mod->constant_count);

    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);
    if (!rt) { printf("ERROR: runtime: %s\n", qvm_error_message(err)); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }
    printf("[4] Runtime creado\n");

    // Registrar funciones nativas
    qvm_ffi_registry *ffi = qvm_runtime_ffi(rt);
    if (!ffi) { printf("ERROR: FFI registry\n"); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    qvm_type_id_t print_args[1] = { QVM_TYPE_I32 };
    qvm_native_function print_func = {
        .name = "print_i32",
        .fn = native_print_i32,
        .return_type = QVM_TYPE_VOID,
        .arg_count = 1,
        .arg_types = print_args
    };

    qvm_native_function answer_func = {
        .name = "get_answer",
        .fn = native_get_answer,
        .return_type = QVM_TYPE_I32,
        .arg_count = 0,
        .arg_types = NULL
    };

    qvm_type_id_t add_args[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_native_function add_func = {
        .name = "add",
        .fn = native_add,
        .return_type = QVM_TYPE_I32,
        .arg_count = 2,
        .arg_types = add_args
    };

    err = qvm_ffi_register(ffi, &print_func);
    if (err != QVM_OK) { printf("ERROR: register print_i32: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    err = qvm_ffi_register(ffi, &answer_func);
    if (err != QVM_OK) { printf("ERROR: register get_answer: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    err = qvm_ffi_register(ffi, &add_func);
    if (err != QVM_OK) { printf("ERROR: register add: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    printf("[5] Funciones nativas registradas: print_i32, get_answer, add\n\n");

    // Crear frame principal
    qvm_type_id_t types[6] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, 
                                QVM_TYPE_VOID, QVM_TYPE_I32, QVM_TYPE_VOID };
    qvm_frame *frame = qvm_frame_create(0, 6, types, NULL, &err);
    if (!frame) { printf("ERROR: frame: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    rt->pc            = 0;
    printf("[6] Frame creado con 6 registros\n\n");

    printf("Ejecutando bytecode que llama a funciones C:\n");
    printf("  1. LOAD_CONST r0 = 10\n");
    printf("  2. LOAD_CONST r1 = 20\n");
    printf("  3. CALL_NATIVE r2 = add(r0, r1)\n");
    printf("  4. CALL_NATIVE print_i32(r2)\n");
    printf("  5. CALL_NATIVE r4 = get_answer()\n");
    printf("  6. CALL_NATIVE print_i32(r4)\n\n");

    err = qvm_runtime_execute(rt);
    if (err != QVM_OK) {
        printf("ERROR: ejecucion: %s\n", qvm_error_message(err));
        qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx);
        return 1;
    }

    printf("\n=== Demo FFI completada ===\n");

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
    return 0;
}
