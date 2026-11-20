#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/qvm.h"

// Función nativa: concatena dos strings y retorna longitud
static qvm_error native_concat(qvm_frame *frame, const qvm_value *args,
                                uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 0) return QVM_ERR_INVALID_ARGUMENT;
    
    const char *s1 = "hello";
    const char *s2 = "world";
    
    qvm_error err;
    qvm_string *str1 = qvm_string_create_from_cstr(s1, &err);
    if (!str1) return err;
    
    qvm_string *str2 = qvm_string_create_from_cstr(s2, &err);
    if (!str2) {
        qvm_string_release(str1);
        return err;
    }
    
    qvm_string *concat = qvm_string_concat(str1, str2, &err);
    qvm_string_release(str1);
    qvm_string_release(str2);
    
    if (!concat) return err;
    
    result->type_id = QVM_TYPE_I32;
    result->payload = qvm_string_len(concat);
    
    qvm_string_release(concat);
    return QVM_OK;
}

// Función nativa: obtiene longitud de string
static qvm_error native_len(qvm_frame *frame, const qvm_value *args,
                             uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 0) return QVM_ERR_INVALID_ARGUMENT;
    
    const char *s = "helloworld";
    qvm_error err;
    qvm_string *str = qvm_string_create_from_cstr(s, &err);
    if (!str) return err;
    
    result->type_id = QVM_TYPE_I32;
    result->payload = qvm_string_len(str);
    
    qvm_string_release(str);
    return QVM_OK;
}

// Función nativa: obtiene carácter en índice
static qvm_error native_char_at(qvm_frame *frame, const qvm_value *args,
                                 uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 1) return QVM_ERR_INVALID_ARGUMENT;
    
    const char *s = "helloworld";
    uint32_t idx = (uint32_t)args[0].payload;
    
    qvm_error err;
    qvm_string *str = qvm_string_create_from_cstr(s, &err);
    if (!str) return err;
    
    int32_t ch = qvm_string_char_at(str, idx, &err);
    qvm_string_release(str);
    
    if (err != QVM_OK) return err;
    
    result->type_id = QVM_TYPE_I32;
    result->payload = ch;
    return QVM_OK;
}

// Función nativa: compara dos strings
static qvm_error native_compare(qvm_frame *frame, const qvm_value *args,
                                 uint32_t arg_count, qvm_value *result) {
    (void)frame;
    if (arg_count != 0) return QVM_ERR_INVALID_ARGUMENT;
    
    const char *s1 = "hello";
    const char *s2 = "world";
    
    qvm_error err;
    qvm_string *str1 = qvm_string_create_from_cstr(s1, &err);
    if (!str1) return err;
    
    qvm_string *str2 = qvm_string_create_from_cstr(s2, &err);
    if (!str2) {
        qvm_string_release(str1);
        return err;
    }
    
    int cmp = qvm_string_compare(str1, str2);
    qvm_string_release(str1);
    qvm_string_release(str2);
    
    result->type_id = QVM_TYPE_BOOL;
    result->payload = (cmp == 0) ? 1 : 0;
    return QVM_OK;
}

// Función nativa: imprime un i32
static qvm_error native_print_i32(qvm_frame *frame, const qvm_value *args,
                                   uint32_t arg_count, qvm_value *result) {
    (void)frame;
    (void)result;
    if (arg_count != 1) return QVM_ERR_INVALID_ARGUMENT;
    printf("  [I32] %lld\n", (long long)args[0].payload);
    result->type_id = QVM_TYPE_VOID;
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

static uint8_t *build_strings_qcb(uint32_t *out_size) {
    uint8_t *d = (uint8_t *)calloc(2048, 1);
    if (!d) return NULL;
    uint32_t o = 0;

    // Header QCB
    write32(d + o, QCB_MAGIC);       o += 4;
    write32(d + o, QCB_VERSION_1_0); o += 4;
    write32(d + o, 0);               o += 4;
    write32(d + o, 2);               o += 4; // 2 secciones

    // Sección 1: Constantes (solo 1 constante: índice 6)
    write32(d + o, QCB_SECTION_CONSTANTS); o += 4;
    write32(d + o, 1 * (uint32_t)sizeof(qvm_value)); o += 4;
    qvm_value c0 = qvm_value_make(QVM_TYPE_I32, 6);
    memcpy(d + o, &c0, sizeof c0); o += (uint32_t)sizeof c0;

    // Sección 2: Bytecode
    write32(d + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    // CALL_NATIVE r0, func 0 (concat), args []
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 0); o += 4; // function_index 0
    d[o++] = 0; // arg_count

    // CALL_NATIVE func 4 (print_i32), args [r0]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 4); o += 4; // function_index 4
    d[o++] = 1; d[o++] = 0;

    // CALL_NATIVE r1, func 1 (len), args []
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 1;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 1); o += 4; // function_index 1
    d[o++] = 0; // arg_count

    // CALL_NATIVE func 4 (print_i32), args [r1]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 4); o += 4; // function_index 4
    d[o++] = 1; d[o++] = 1;

    // LOAD_CONST r2, const 0 (valor 6 para char_at)
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 2;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 0); o += 4; // índice de constante 0

    // CALL_NATIVE r3, func 2 (char_at), args [r2]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 3;
    write16(d + o, QVM_TYPE_I32); o += 2;
    write32(d + o, 2); o += 4; // function_index 2
    d[o++] = 1; d[o++] = 2;

    // CALL_NATIVE func 4 (print_i32), args [r3]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 4); o += 4; // function_index 4
    d[o++] = 1; d[o++] = 3;

    // CALL_NATIVE r4, func 3 (compare), args []
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 4;
    write16(d + o, QVM_TYPE_BOOL); o += 2;
    write32(d + o, 3); o += 4; // function_index 3
    d[o++] = 0; // arg_count

    // CALL_NATIVE func 4 (print_i32), args [r4]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 4); o += 4; // function_index 4
    d[o++] = 1; d[o++] = 4;

    // RETURN_VOID
    d[o++] = QVM_OP_RETURN_VOID;
    d[o++] = 0; d[o++] = 0; d[o++] = 0;

    write32(d + size_at, o - start);
    *out_size = o;
    return d;
}

int main(void) {
    printf("=== QVM Demo: Operaciones con Strings (vía FFI) ===\n\n");

    uint32_t qcb_size = 0;
    uint8_t *qcb = build_strings_qcb(&qcb_size);
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
    
    qvm_native_function concat_func = {
        .name = "concat",
        .fn = native_concat,
        .return_type = QVM_TYPE_I32,
        .arg_count = 0,
        .arg_types = NULL
    };
    
    qvm_native_function len_func = {
        .name = "len",
        .fn = native_len,
        .return_type = QVM_TYPE_I32,
        .arg_count = 0,
        .arg_types = NULL
    };
    
    qvm_type_id_t char_at_args[1] = { QVM_TYPE_I32 };
    qvm_native_function char_at_func = {
        .name = "char_at",
        .fn = native_char_at,
        .return_type = QVM_TYPE_I32,
        .arg_count = 1,
        .arg_types = char_at_args
    };
    
    qvm_native_function compare_func = {
        .name = "compare",
        .fn = native_compare,
        .return_type = QVM_TYPE_BOOL,
        .arg_count = 0,
        .arg_types = NULL
    };
    
    qvm_type_id_t print_i32_args[1] = { QVM_TYPE_I32 };
    qvm_native_function print_i32_func = {
        .name = "print_i32",
        .fn = native_print_i32,
        .return_type = QVM_TYPE_VOID,
        .arg_count = 1,
        .arg_types = print_i32_args
    };

    qvm_ffi_register(ffi, &concat_func);
    qvm_ffi_register(ffi, &len_func);
    qvm_ffi_register(ffi, &char_at_func);
    qvm_ffi_register(ffi, &compare_func);
    qvm_ffi_register(ffi, &print_i32_func);
    printf("[5] Funciones nativas registradas (5 funciones)\n\n");

    // Crear frame principal
    qvm_type_id_t types[5] = {
        QVM_TYPE_I32,  // r0: longitud de concat
        QVM_TYPE_I32,  // r1: longitud de string
        QVM_TYPE_I32,  // r2: índice para char_at
        QVM_TYPE_I32,  // r3: carácter
        QVM_TYPE_BOOL  // r4: resultado de comparación
    };
    qvm_frame *frame = qvm_frame_create(0, 5, types, NULL, &err);
    if (!frame) { printf("ERROR: frame: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    rt->pc            = 0;
    printf("[6] Frame creado con 5 registros\n\n");

    printf("Ejecutando operaciones con strings (todas vía FFI):\n");
    printf("  1. Concatenar \"hello\" + \"world\" -> longitud\n");
    printf("  2. Imprimir longitud\n");
    printf("  3. Obtener longitud de \"helloworld\"\n");
    printf("  4. Imprimir longitud\n");
    printf("  5. Obtener carácter en índice 6\n");
    printf("  6. Imprimir carácter (como código ASCII)\n");
    printf("  7. Comparar \"hello\" == \"world\"\n");
    printf("  8. Imprimir resultado\n\n");

    err = qvm_runtime_execute(rt);
    if (err != QVM_OK) {
        printf("ERROR: ejecucion: %s\n", qvm_error_message(err));
        qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx);
        return 1;
    }

    printf("\n=== Demo de Strings completada ===\n");
    printf("\nNota: Todas las operaciones de strings se ejecutan en C vía FFI.\n");
    printf("Esto demuestra la interoperabilidad completa entre QVM y código nativo.\n");

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
    return 0;
}
