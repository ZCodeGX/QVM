#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "qvm/qit.h"
#include "qvm/instruction.h"
#include "qvm/value.h"

_Static_assert(sizeof(qvm_value) == 16, "ABI: qvm_value debe ser 16 bytes");

/* Forward declarations de codebuf y reloc */
typedef struct codebuf_t codebuf_t;
typedef struct reloc_t reloc_t;

codebuf_t *codebuf_create(void);
void codebuf_destroy(codebuf_t *cb);
void codebuf_emit8(codebuf_t *cb, uint8_t value);
void codebuf_emit32(codebuf_t *cb, uint32_t value);
size_t codebuf_tell(codebuf_t *cb);
void codebuf_patch32(codebuf_t *cb, size_t offset, uint32_t value);
uint8_t *codebuf_data(codebuf_t *cb);
size_t codebuf_size(codebuf_t *cb);
int codebuf_has_overflow(codebuf_t *cb);
void codebuf_seal(codebuf_t *cb);

reloc_t *reloc_create(void);
void reloc_destroy(reloc_t *r);
int reloc_define_label(reloc_t *r, uint32_t pc, size_t code_offset);
int reloc_add_patch_rel32(reloc_t *r, size_t patch_offset, uint32_t target_pc);
int reloc_resolve(reloc_t *r, uint8_t *code, size_t code_size, uint32_t bytecode_size);

/* Estructura del compilador QIT */
struct qvm_qit {
    const qvm_context *ctx;
    int debug_enabled;
};

/* Código compilado */
struct qvm_compiled_code {
    void *code;
    size_t size;
    size_t capacity;
    const char *backend;
};

/* Wrappers deprecated */
struct qit_code {
    void *code;
    size_t size;
    size_t capacity;
};

/* Implementación de la API nueva */
qvm_qit *qvm_qit_create(const qvm_context *ctx, qvm_error *err) {
    qvm_qit *qit = calloc(1, sizeof(qvm_qit));
    if (!qit) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    qit->ctx = ctx;
    qit->debug_enabled = 0;
    return qit;
}

void qvm_qit_destroy(qvm_qit *qit) {
    free(qit);
}

void qvm_qit_set_debug(qvm_qit *qit, int enabled) {
    if (qit) qit->debug_enabled = enabled;
}

static void emit_x86_prologue(codebuf_t *cb) {
    codebuf_emit8(cb, 0x55);                    // push rbp
    codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x89); codebuf_emit8(cb, 0xe5);  // mov rbp, rsp
    codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x8b); codebuf_emit8(cb, 0x7f); codebuf_emit8(cb, 0x18);  // mov rdi, [rdi+24]
}

static void emit_load_reg(codebuf_t *cb, uint8_t reg) {
    codebuf_emit8(cb, 0x48);
    codebuf_emit8(cb, 0x8b);
    codebuf_emit8(cb, 0x87);
    codebuf_emit32(cb, (uint32_t)reg * 16u);
}

static void emit_store_reg(codebuf_t *cb, uint8_t reg) {
    codebuf_emit8(cb, 0x48);
    codebuf_emit8(cb, 0x89);
    codebuf_emit8(cb, 0x87);
    codebuf_emit32(cb, (uint32_t)reg * 16u);
}

const qvm_compiled_code *qvm_qit_compile(qvm_qit *qit, 
                                          const qvm_module *module, 
                                          uint32_t function_index, 
                                          qvm_error *err) {
    if (!qit || !module || !module->bytecode) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    
    codebuf_t *cb = codebuf_create();
    if (!cb) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    reloc_t *reloc = reloc_create();
    if (!reloc) {
        codebuf_destroy(cb);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    emit_x86_prologue(cb);
    
    const uint8_t *bc = module->bytecode;
    uint32_t size = module->bytecode_size;
    uint32_t pc = 0;
    int saw_return = 0;
    
    while (pc < size) {
        // Definir label para este PC
        if (!reloc_define_label(reloc, pc, codebuf_tell(cb))) {
            codebuf_destroy(cb);
            reloc_destroy(reloc);
            if (err) *err = QVM_ERR_LIMIT_EXCEEDED;
            return NULL;
        }
        
        uint8_t op = bc[pc];
        
        switch (op) {
            case QVM_OP_ARITH_ADD: {
                if (pc + 6 > size) goto fail;
                uint8_t dst = bc[pc + 1];
                uint8_t src1 = bc[pc + 4];
                uint8_t src2 = bc[pc + 5];
                
                emit_load_reg(cb, src1);
                codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x89); codebuf_emit8(cb, 0xc1);  // mov rcx, rax
                emit_load_reg(cb, src2);
                codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x01); codebuf_emit8(cb, 0xc8);  // add rax, rcx
                emit_store_reg(cb, dst);
                pc += 6;
                break;
            }
            
            case QVM_OP_CMP_LT: {
                if (pc + 6 > size) goto fail;
                uint8_t dst = bc[pc + 1];
                uint8_t src1 = bc[pc + 4];
                uint8_t src2 = bc[pc + 5];
                
                emit_load_reg(cb, src1);
                codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x89); codebuf_emit8(cb, 0xc1);
                emit_load_reg(cb, src2);
                codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x39); codebuf_emit8(cb, 0xc1);
                codebuf_emit8(cb, 0x0f); codebuf_emit8(cb, 0x92); codebuf_emit8(cb, 0xc0);  // setb al
                codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x0f); codebuf_emit8(cb, 0xb6); codebuf_emit8(cb, 0xc0);  // movzx rax, al
                emit_store_reg(cb, dst);
                pc += 6;
                break;
            }
            
            case QVM_OP_JMP_IF: {
                if (pc + 9 > size) goto fail;
                uint8_t src = bc[pc + 4];
                int32_t target_bc = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
                
                emit_load_reg(cb, src);
                codebuf_emit8(cb, 0x48); codebuf_emit8(cb, 0x85); codebuf_emit8(cb, 0xc0);  // test rax, rax
                codebuf_emit8(cb, 0x0f); codebuf_emit8(cb, 0x85);  // jnz rel32
                
                size_t patch_offset = codebuf_tell(cb);
                codebuf_emit32(cb, 0);  // Placeholder
                
                if (!reloc_add_patch_rel32(reloc, patch_offset, target_bc)) {
                    goto fail;
                }
                
                pc += 9;
                break;
            }
            
            case QVM_OP_RETURN: {
                if (pc + 5 > size) goto fail;
                uint8_t src = bc[pc + 4];  // Semántica canónica: src1
                emit_load_reg(cb, src);
                codebuf_emit8(cb, 0x5d);  // pop rbp
                codebuf_emit8(cb, 0xc3);  // ret
                saw_return = 1;
                pc += 5;
                break;
            }
            
            default:
                goto fail;  // Opcode no soportado
        }
        
        if (codebuf_has_overflow(cb)) {
            goto fail;
        }
    }
    
    if (!saw_return) {
        goto fail;  // Sin RETURN
    }
    
    codebuf_seal(cb);
    
    // Resolver reubicaciones
    if (!reloc_resolve(reloc, codebuf_data(cb), codebuf_size(cb), size)) {
        goto fail;
    }
    
    // Asignar memoria ejecutable
    size_t code_size = codebuf_size(cb);
    size_t capacity = 4096;  // Alinear a página
    void *exec_mem = mmap(NULL, capacity, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (exec_mem == MAP_FAILED) {
        goto fail;
    }
    
    memcpy(exec_mem, codebuf_data(cb), code_size);
    
    if (mprotect(exec_mem, capacity, PROT_READ | PROT_EXEC) != 0) {
        munmap(exec_mem, capacity);
        goto fail;
    }
    
    if (qit->debug_enabled) {
        fprintf(stderr, "QIT: %zu bytes emitidos\n", code_size);
        uint8_t *data = (uint8_t *)exec_mem;
        for (size_t i = 0; i < code_size; i++) {
            fprintf(stderr, "%02x ", data[i]);
            if ((i + 1) % 16 == 0) fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
    
    codebuf_destroy(cb);
    reloc_destroy(reloc);
    
    qvm_compiled_code *code = calloc(1, sizeof(qvm_compiled_code));
    if (!code) {
        munmap(exec_mem, capacity);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    code->code = exec_mem;
    code->size = code_size;
    code->capacity = capacity;
    code->backend = "x86_64_sysv";
    
    if (err) *err = QVM_OK;
    return code;
    
fail:
    codebuf_destroy(cb);
    reloc_destroy(reloc);
    if (err) *err = QVM_ERR_INVALID_BYTECODE;
    return NULL;
}

qvm_entry_fn qvm_qit_entry(const qvm_compiled_code *code) {
    return code ? (qvm_entry_fn)code->code : NULL;
}

size_t qvm_qit_size(const qvm_compiled_code *code) {
    return code ? code->size : 0;
}

const char *qvm_qit_backend(const qvm_compiled_code *code) {
    return code ? code->backend : NULL;
}

void qvm_qit_release(const qvm_compiled_code *code) {
    if (code) {
        if (code->code) {
            munmap(code->code, code->capacity);
        }
        free((void *)code);
    }
}

/* ========================================================================
   Wrappers deprecated para compatibilidad con API antigua
   ======================================================================== */

static int qit_debug_global = 0;

void qit_set_debug(int enabled) {
    qit_debug_global = enabled;
}

qit_code *qit_compile(const uint8_t *bytecode, uint32_t size) {
    if (!bytecode || size == 0) return NULL;
    
    // Crear módulo temporal
    qvm_module *mod = calloc(1, sizeof(qvm_module));
    if (!mod) return NULL;
    
    mod->bytecode = bytecode;
    mod->bytecode_size = size;
    
    // Crear QIT temporal
    qvm_error err;
    qvm_qit *qit = qvm_qit_create(NULL, &err);
    if (!qit) {
        free(mod);
        return NULL;
    }
    
    qvm_qit_set_debug(qit, qit_debug_global);
    
    const qvm_compiled_code *compiled = qvm_qit_compile(qit, mod, 0, &err);
    
    qvm_qit_destroy(qit);
    free(mod);
    
    if (!compiled) return NULL;
    
    // Convertir a formato antiguo
    qit_code *old_code = malloc(sizeof(qit_code));
    if (!old_code) {
        qvm_qit_release(compiled);
        return NULL;
    }
    
    old_code->code = compiled->code;
    old_code->size = compiled->size;
    old_code->capacity = compiled->capacity;
    
    // Liberar el wrapper (no el código)
    free((void *)compiled);
    
    return old_code;
}

void qit_destroy(qit_code *code) {
    if (code) {
        if (code->code) {
            munmap(code->code, code->capacity);
        }
        free(code);
    }
}

void *qit_get_entry(qit_code *code) {
    return code ? code->code : NULL;
}

size_t qit_get_size(qit_code *code) {
    return code ? code->size : 0;
}
