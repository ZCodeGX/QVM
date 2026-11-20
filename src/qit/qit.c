#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "qvm/qit.h"
#include "qvm/instruction.h"

static int qit_debug = 0;

// Estructura para guardar parches pendientes
typedef struct {
    uint32_t patch_pos;
    uint32_t target_bc;
} qit_patch_t;

static qit_patch_t patches[256];
static int patch_count = 0;

struct qit_code {
    void *code;
    size_t size;
    size_t capacity;
};

static uint32_t offset_map[1024];

void qit_set_debug(int enabled) {
    qit_debug = enabled;
}

static inline void emit_byte(qit_code *qit, uint8_t byte) {
    if (qit->size >= qit->capacity) {
        fprintf(stderr, "QIT: Buffer overflow\n");
        return;
    }
    ((uint8_t *)qit->code)[qit->size++] = byte;
}

static inline size_t get_offset(qit_code *qit) {
    return qit->size;
}

static void emit_load_reg(qit_code *qit, uint8_t reg) {
    emit_byte(qit, 0x48);
    emit_byte(qit, 0x8b);
    emit_byte(qit, 0x87);
    emit_byte(qit, (uint8_t)(reg * 16));
    emit_byte(qit, 0x00);
    emit_byte(qit, 0x00);
    emit_byte(qit, 0x00);
}

static void emit_store_reg(qit_code *qit, uint8_t reg) {
    emit_byte(qit, 0x48);
    emit_byte(qit, 0x89);
    emit_byte(qit, 0x87);
    emit_byte(qit, (uint8_t)(reg * 16));
    emit_byte(qit, 0x00);
    emit_byte(qit, 0x00);
    emit_byte(qit, 0x00);
}

static void compile_arith_add(qit_code *qit, const uint8_t *insn) {
    uint8_t dst = insn[1];
    uint8_t src1 = insn[4];
    uint8_t src2 = insn[5];
    
    emit_load_reg(qit, src1);
    emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0xc1); // mov rcx, rax
    emit_load_reg(qit, src2);
    emit_byte(qit, 0x48); emit_byte(qit, 0x01); emit_byte(qit, 0xc8); // add rax, rcx
    emit_store_reg(qit, dst);
}

static void compile_cmp_lt(qit_code *qit, const uint8_t *insn) {
    uint8_t dst = insn[1];
    uint8_t src1 = insn[4];
    uint8_t src2 = insn[5];
    
    emit_load_reg(qit, src1);
    emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0xc1); // mov rcx, rax
    emit_load_reg(qit, src2);
    emit_byte(qit, 0x48); emit_byte(qit, 0x39); emit_byte(qit, 0xc1); // cmp rcx, rax
    emit_byte(qit, 0x0f); emit_byte(qit, 0x92); emit_byte(qit, 0xc0); // setb al
    emit_byte(qit, 0x48); emit_byte(qit, 0x0f); emit_byte(qit, 0xb6); emit_byte(qit, 0xc0); // movzx rax, al
    emit_store_reg(qit, dst);
}

static void compile_jmp_if(qit_code *qit, const uint8_t *insn, uint32_t pc) {
    uint8_t src = insn[4];
    int32_t target_bc = insn[5] | (insn[6] << 8) | (insn[7] << 16) | (insn[8] << 24);
    
    emit_load_reg(qit, src);
    emit_byte(qit, 0x48); emit_byte(qit, 0x85); emit_byte(qit, 0xc0); // test rax, rax
    emit_byte(qit, 0x0f); emit_byte(qit, 0x85); // jnz rel32
    
    // Registrar parche para después
    patches[patch_count].patch_pos = get_offset(qit);
    patches[patch_count].target_bc = target_bc;
    patch_count++;
    
    // Placeholder
    emit_byte(qit, 0x00); emit_byte(qit, 0x00); emit_byte(qit, 0x00); emit_byte(qit, 0x00);
}

static void compile_return(qit_code *qit, const uint8_t *insn) {
    uint8_t src = insn[1];
    emit_load_reg(qit, src);
    emit_byte(qit, 0x5d); // pop rbp
    emit_byte(qit, 0xc3); // ret
}

qit_code *qit_compile(const uint8_t *bytecode, uint32_t size) {
    if (!bytecode || size == 0) return NULL;
    
    size_t capacity = 4096;
    void *code = mmap(NULL, capacity, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) return NULL;
    
    qit_code *qit = malloc(sizeof(qit_code));
    if (!qit) {
        munmap(code, capacity);
        return NULL;
    }
    
    qit->code = code;
    qit->size = 0;
    qit->capacity = capacity;
    
    memset(offset_map, 0, sizeof(offset_map));
    patch_count = 0;
    
    // Prólogo
    emit_byte(qit, 0x55);                    // push rbp
    emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0xe5);  // mov rbp, rsp
    emit_byte(qit, 0x48); emit_byte(qit, 0x8b); emit_byte(qit, 0x7f); emit_byte(qit, 0x18);  // mov rdi, [rdi+24]
    
    uint32_t pc = 0;
    while (pc < size) {
        offset_map[pc] = get_offset(qit);
        uint8_t op = bytecode[pc];
        
        switch (op) {
            case QVM_OP_ARITH_ADD:
                compile_arith_add(qit, bytecode + pc);
                pc += 6;
                break;
            case QVM_OP_CMP_LT:
                compile_cmp_lt(qit, bytecode + pc);
                pc += 6;
                break;
            case QVM_OP_JMP_IF:
                compile_jmp_if(qit, bytecode + pc, pc);
                pc += 9;
                break;
            case QVM_OP_RETURN:
                compile_return(qit, bytecode + pc);
                pc += 5;
                goto done;
            default:
                free(qit);
                munmap(code, capacity);
                return NULL;
        }
    }
    
done:
    // Aplicar parches registrados correctamente
    for (int i = 0; i < patch_count; i++) {
        uint32_t patch_pos = patches[i].patch_pos;
        uint32_t target_bc = patches[i].target_bc;
        uint32_t target_gen = offset_map[target_bc];
        
        int32_t offset = target_gen - (patch_pos + 4);
        uint8_t *patch = (uint8_t *)code + patch_pos;
        patch[0] = offset & 0xff;
        patch[1] = (offset >> 8) & 0xff;
        patch[2] = (offset >> 16) & 0xff;
        patch[3] = (offset >> 24) & 0xff;
        
        if (qit_debug) {
            printf("QIT: Patch JMP_IF at %u -> target_gen %u (offset %d)\n", 
                   patch_pos, target_gen, offset);
        }
    }
    
    if (qit_debug) {
        printf("QIT Generated Code (%zu bytes):\n", qit->size);
        for (size_t i = 0; i < qit->size; i++) {
            printf("%02x ", ((uint8_t *)code)[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }
    
    mprotect(code, capacity, PROT_READ | PROT_EXEC);
    return qit;
}

void qit_destroy(qit_code *code) {
    if (code) {
        if (code->code) munmap(code->code, code->capacity);
        free(code);
    }
}

void *qit_get_entry(qit_code *code) {
    return code ? code->code : NULL;
}

size_t qit_get_size(qit_code *code) {
    return code ? code->size : 0;
}
