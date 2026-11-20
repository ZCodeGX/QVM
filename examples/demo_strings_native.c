#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/qvm.h"

// Función nativa: imprime un string
static qvm_error native_print_string(qvm_frame *frame, const qvm_value *args,
                                      uint32_t arg_count, qvm_value *result) {
    (void)frame;
    (void)result;
    if (arg_count != 1) return QVM_ERR_INVALID_ARGUMENT;
    
    qvm_string *str;
    memcpy(&str, &args[0].payload, sizeof(qvm_string *));
    
    if (str && str->data) {
        printf("  [STRING] \"%.*s\" (len=%u)\n", str->len, str->data, str->len);
    } else {
        printf("  [STRING] <null>\n");
    }
    
    result->type_id = QVM_TYPE_VOID;
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

// Escribe un string en formato QCB: type_id(2) + len(4) + data(len)
static uint32_t write_string_qcb(uint8_t *buf, const char *str) {
    uint32_t offset = 0;
    uint16_t type_id = QVM_TYPE_STRING;
    uint32_t len = (uint32_t)strlen(str);
    
    memcpy(buf + offset, &type_id, 2); offset += 2;
    memcpy(buf + offset, &len, 4); offset += 4;
    memcpy(buf + offset, str, len); offset += len;
    
    return offset;
}

static uint8_t *build_strings_native_qcb(uint32_t *out_size) {
    uint8_t *d = (uint8_t *)calloc(4096, 1);
    if (!d) return NULL;
    uint32_t o = 0;

    // Header QCB
    write32(d + o, QCB_MAGIC);       o += 4;
    write32(d + o, QCB_VERSION_1_0); o += 4;
    write32(d + o, 0);               o += 4;
    write32(d + o, 2);               o += 4; // 2 secciones

    // Sección 1: Constantes (strings embebidos en formato QCB)
    write32(d + o, QCB_SECTION_CONSTANTS); o += 4;
    uint32_t const_size_offset = o;
    write32(d + o, 0); o += 4; // tamaño placeholder
    uint32_t const_start = o;
    
    // Constante 0: string "hello"
    o += write_string_qcb(d + o, "hello");
    
    // Constante 1: string "world"
    o += write_string_qcb(d + o, "world");
    
    // Constante 2: string "QVM"
    o += write_string_qcb(d + o, "QVM");
    
    write32(d + const_size_offset, o - const_start);

    // Sección 2: Bytecode
    write32(d + o, QCB_SECTION_BYTECODE); o += 4;
    uint32_t size_at = o; o += 4;
    uint32_t start = o;

    // LOAD_CONST r0, const 0 (string "hello")
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 0;
    write16(d + o, QVM_TYPE_STRING); o += 2;
    write32(d + o, 0); o += 4;

    // CALL_NATIVE func 0 (print_string), args [r0]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 0); o += 4;
    d[o++] = 1; d[o++] = 0;

    // LOAD_CONST r1, const 1 (string "world")
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 1;
    write16(d + o, QVM_TYPE_STRING); o += 2;
    write32(d + o, 1); o += 4;

    // CALL_NATIVE func 0 (print_string), args [r1]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 0); o += 4;
    d[o++] = 1; d[o++] = 1;

    // STR_CONCAT r2, r0, r1 (concatenar "hello" + "world")
    d[o++] = QVM_OP_STR_CONCAT; d[o++] = 2;
    write16(d + o, QVM_TYPE_STRING); o += 2;
    d[o++] = 0; d[o++] = 1;

    // CALL_NATIVE func 0 (print_string), args [r2]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 0); o += 4;
    d[o++] = 1; d[o++] = 2;

    // STR_LEN r3, r2 (longitud de "helloworld")
    d[o++] = QVM_OP_STR_LEN; d[o++] = 3;
    write16(d + o, QVM_TYPE_U32); o += 2;
    d[o++] = 2;

    // CALL_NATIVE func 1 (print_i32), args [r3]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 1); o += 4;
    d[o++] = 1; d[o++] = 3;

    // LOAD_CONST r4, const 2 (string "QVM")
    d[o++] = QVM_OP_LOAD_CONST; d[o++] = 4;
    write16(d + o, QVM_TYPE_STRING); o += 2;
    write32(d + o, 2); o += 4;

    // CALL_NATIVE func 0 (print_string), args [r4]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 0); o += 4;
    d[o++] = 1; d[o++] = 4;

    // STR_CMP r5, r0, r4 (comparar "hello" == "QVM")
    d[o++] = QVM_OP_STR_CMP; d[o++] = 5;
    write16(d + o, QVM_TYPE_BOOL); o += 2;
    d[o++] = 0; d[o++] = 4;

    // CALL_NATIVE func 1 (print_i32), args [r5]
    d[o++] = QVM_OP_CALL_NATIVE; d[o++] = 0xFF;
    write16(d + o, QVM_TYPE_VOID); o += 2;
    write32(d + o, 1); o += 4;
    d[o++] = 1; d[o++] = 5;

    // RETURN_VOID
    d[o++] = QVM_OP_RETURN_VOID;
    d[o++] = 0; d[o++] = 0; d[o++] = 0;

    write32(d + size_at, o - start);
    *out_size = o;
    return d;
}

int main(void) {
    printf("=== QVM Demo: Strings Nativos en QCB ===\n\n");

    uint32_t qcb_size = 0;
    uint8_t *qcb = build_strings_native_qcb(&qcb_size);
    if (!qcb) { printf("ERROR: build QCB\n"); return 1; }
    printf("[1] QCB construido en memoria (%u bytes)\n", qcb_size);
    printf("    Contiene 3 strings embebidos: \"hello\", \"world\", \"QVM\"\n\n");

    qvm_error err;
    qvm_context *ctx = qvm_context_create(NULL, &err);
    if (!ctx) { printf("ERROR: contexto: %s\n", qvm_error_message(err)); free(qcb); return 1; }
    printf("[2] Contexto QVM creado\n");

    qvm_module *mod = qvm_module_load_from_memory(qcb, qcb_size, ctx, &err);
    free(qcb);
    if (!mod) { printf("ERROR: loader: %s\n", qvm_error_message(err)); qvm_context_destroy(ctx); return 1; }
    printf("[3] Modulo cargado (%u constantes, %u bytes de bytecode)\n", 
           mod->constant_count, mod->bytecode_size);
    printf("    Strings cargados automáticamente por el loader\n\n");

    qvm_runtime *rt = qvm_runtime_create(ctx, mod, &err);
    if (!rt) { printf("ERROR: runtime: %s\n", qvm_error_message(err)); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }
    printf("[4] Runtime creado\n");

    // Registrar funciones nativas
    qvm_ffi_registry *ffi = qvm_runtime_ffi(rt);
    
    qvm_type_id_t print_str_args[1] = { QVM_TYPE_STRING };
    qvm_native_function print_str_func = {
        .name = "print_string",
        .fn = native_print_string,
        .return_type = QVM_TYPE_VOID,
        .arg_count = 1,
        .arg_types = print_str_args
    };
    
    qvm_type_id_t print_i32_args[1] = { QVM_TYPE_I32 };
    qvm_native_function print_i32_func = {
        .name = "print_i32",
        .fn = native_print_i32,
        .return_type = QVM_TYPE_VOID,
        .arg_count = 1,
        .arg_types = print_i32_args
    };

    qvm_ffi_register(ffi, &print_str_func);
    qvm_ffi_register(ffi, &print_i32_func);
    printf("[5] Funciones nativas registradas (2 funciones)\n\n");

    // Crear frame principal
    qvm_type_id_t types[6] = {
        QVM_TYPE_STRING,  // r0: "hello"
        QVM_TYPE_STRING,  // r1: "world"
        QVM_TYPE_STRING,  // r2: "helloworld" (concatenado)
        QVM_TYPE_U32,     // r3: longitud
        QVM_TYPE_STRING,  // r4: "QVM"
        QVM_TYPE_BOOL     // r5: resultado de comparación
    };
    qvm_frame *frame = qvm_frame_create(0, 6, types, NULL, &err);
    if (!frame) { printf("ERROR: frame: %s\n", qvm_error_message(err)); qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx); return 1; }

    rt->call_stack[0] = frame;
    rt->call_depth    = 1;
    rt->pc            = 0;
    printf("[6] Frame creado con 6 registros\n\n");

    printf("Ejecutando operaciones con strings nativos:\n");
    printf("  1. Cargar string \"hello\" desde QCB -> r0\n");
    printf("  2. Imprimir r0\n");
    printf("  3. Cargar string \"world\" desde QCB -> r1\n");
    printf("  4. Imprimir r1\n");
    printf("  5. Concatenar r0 + r1 -> r2\n");
    printf("  6. Imprimir r2\n");
    printf("  7. Obtener longitud de r2 -> r3\n");
    printf("  8. Imprimir r3\n");
    printf("  9. Cargar string \"QVM\" desde QCB -> r4\n");
    printf(" 10. Imprimir r4\n");
    printf(" 11. Comparar r0 == r4 -> r5\n");
    printf(" 12. Imprimir r5\n\n");

    err = qvm_runtime_execute(rt);
    if (err != QVM_OK) {
        printf("ERROR: ejecucion: %s\n", qvm_error_message(err));
        qvm_runtime_destroy(rt); qvm_module_destroy(mod); qvm_context_destroy(ctx);
        return 1;
    }

    printf("\n=== Demo de Strings Nativos completada ===\n");
    printf("\nNota: Los strings se cargaron directamente desde el archivo QCB.\n");
    printf("El loader creó los objetos qvm_string automáticamente.\n");

    qvm_runtime_destroy(rt);
    qvm_module_destroy(mod);
    qvm_context_destroy(ctx);
    return 0;
}
