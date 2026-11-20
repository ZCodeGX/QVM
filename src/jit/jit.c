#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include "qvm/runtime.h"
#include "qvm/instruction.h"
#include "qvm/jit.h"

qvm_jit_code *qvm_jit_compile(const uint8_t *bytecode, uint32_t size) {
    size_t code_size = 4096;
    void *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) return NULL;
    
    uint8_t *p = (uint8_t *)code;
    uint32_t pc = 0;
    
    // Tabla para mapear offsets de bytecode a offsets de código generado
    uint32_t bc_to_gen[256] = {0};
    
    // Prólogo (6 bytes)
    bc_to_gen[0] = 0;
    *p++ = 0x55;                    // push rbp
    *p++ = 0x48; *p++ = 0x89; *p++ = 0xe5;  // mov rbp, rsp
    *p++ = 0x48; *p++ = 0x8b; *p++ = 0x7f; *p++ = 0x18;  // mov rdi, [rdi+24]
    
    while (pc < size) {
        uint8_t op = bytecode[pc];
        bc_to_gen[pc] = p - (uint8_t *)code;
        
        if (op == QVM_OP_ARITH_ADD) {
            uint8_t dst = bytecode[pc + 1];
            uint8_t src1 = bytecode[pc + 4];
            uint8_t src2 = bytecode[pc + 5];
            
            *p++ = 0x48; *p++ = 0x8b; *p++ = 0x87;
            *p++ = (uint8_t)(src1 * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            *p++ = 0x48; *p++ = 0x03; *p++ = 0x87;
            *p++ = (uint8_t)(src2 * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            *p++ = 0x48; *p++ = 0x89; *p++ = 0x87;
            *p++ = (uint8_t)(dst * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            pc += 6;
        }
        else if (op == QVM_OP_CMP_LT) {
            uint8_t dst = bytecode[pc + 1];
            uint8_t src1 = bytecode[pc + 4];
            uint8_t src2 = bytecode[pc + 5];
            
            *p++ = 0x48; *p++ = 0x8b; *p++ = 0x87;
            *p++ = (uint8_t)(src1 * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            *p++ = 0x48; *p++ = 0x3b; *p++ = 0x87;
            *p++ = (uint8_t)(src2 * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            *p++ = 0x0f; *p++ = 0x92; *p++ = 0xc0;  // setb al
            *p++ = 0x48; *p++ = 0x0f; *p++ = 0xb6; *p++ = 0xc0;  // movzx rax, al
            
            *p++ = 0x48; *p++ = 0x89; *p++ = 0x87;
            *p++ = (uint8_t)(dst * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            pc += 6;
        }
        else if (op == QVM_OP_JMP_IF) {
            uint8_t src = bytecode[pc + 4];
            int32_t target_bc = bytecode[pc + 5] | (bytecode[pc + 6] << 8) | 
                               (bytecode[pc + 7] << 16) | (bytecode[pc + 8] << 24);
            
            *p++ = 0x48; *p++ = 0x8b; *p++ = 0x87;
            *p++ = (uint8_t)(src * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            *p++ = 0x48; *p++ = 0x85; *p++ = 0xc0;  // test rax, rax
            
            // Calcular offset relativo al target en código generado
            int32_t gen_target = bc_to_gen[target_bc];
            int32_t current_gen = p - (uint8_t *)code;
            int32_t offset = gen_target - current_gen - 2;
            
            *p++ = 0x75;  // jnz rel8
            *p++ = (uint8_t)offset;
            
            pc += 9;
        }
        else if (op == QVM_OP_RETURN) {
            uint8_t src = bytecode[pc + 1];
            
            *p++ = 0x48; *p++ = 0x8b; *p++ = 0x87;
            *p++ = (uint8_t)(src * 16); *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
            
            *p++ = 0x5d;  // pop rbp
            *p++ = 0xc3;  // ret
            
            pc += 5;
            break;
        }
        else {
            munmap(code, code_size);
            return NULL;
        }
    }
    
    mprotect(code, code_size, PROT_READ | PROT_EXEC);
    
    qvm_jit_code *jit = malloc(sizeof(qvm_jit_code));
    jit->code = code;
    jit->size = code_size;
    jit->entry_point = 0;
    
    return jit;
}

void qvm_jit_destroy(qvm_jit_code *jit) {
    if (jit) {
        munmap(jit->code, jit->size);
        free(jit);
    }
}
