#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/verifier.h"
#include "qvm/instruction.h"

typedef struct qvm_reg_state {
    uint32_t size;
    uint8_t  is_owned;
    uint8_t  is_valid;
} qvm_reg_state;

qvm_error qvm_verify_qms_module(const uint8_t *bytecode, uint32_t size, uint32_t reg_count) {
    if (!bytecode || reg_count == 0) return QVM_ERR_INVALID_ARGUMENT;

    qvm_reg_state *regs = (qvm_reg_state *)calloc(reg_count, sizeof(qvm_reg_state));
    if (!regs) return QVM_ERR_OUT_OF_MEMORY;

    uint32_t pc = 0;
    while (pc < size) {
        uint8_t opcode = bytecode[pc];
        
        switch (opcode) {
            case QVM_OP_ALLOC: {
                if (pc + 8 > size) { free(regs); return QVM_ERR_INVALID_BYTECODE; }
                
                uint8_t dst = bytecode[pc + 1];
                uint32_t alloc_size = ((uint32_t)bytecode[pc+2] << 24) | ((uint32_t)bytecode[pc+3] << 16) | 
                                      ((uint32_t)bytecode[pc+4] << 8) | bytecode[pc+5];
                
                if (dst >= reg_count) { free(regs); return QVM_ERR_INVALID_ARGUMENT; }
                
                if (regs[dst].is_owned && regs[dst].size > 0) {
                    printf("[VERIFIER] Error en PC=%u: Memory Leak detectado en registro r%u\n", pc, dst);
                    free(regs);
                    return QVM_ERR_MEMORY_LEAK;
                }
                
                regs[dst].size = alloc_size;
                regs[dst].is_owned = 1;
                regs[dst].is_valid = 1;
                
                pc += 8;
                break;
            }
            
            case QVM_OP_FREE: {
                if (pc + 4 > size) { free(regs); return QVM_ERR_INVALID_BYTECODE; }
                
                uint8_t src = bytecode[pc + 1];
                if (src >= reg_count) { free(regs); return QVM_ERR_INVALID_ARGUMENT; }
                
                if (!regs[src].is_valid || !regs[src].is_owned) {
                    printf("[VERIFIER] Error en PC=%u: Double Free o Use-After-Free en r%u\n", pc, src);
                    free(regs);
                    return QVM_ERR_DOUBLE_FREE;
                }
                
                regs[src].is_owned = 0;
                regs[src].size = 0;
                
                pc += 4;
                break;
            }
            
            case QVM_OP_LOAD_PTR: {
                if (pc + 8 > size) { free(regs); return QVM_ERR_INVALID_BYTECODE; }
                
                uint8_t dst = bytecode[pc + 1];
                uint8_t src_ptr = bytecode[pc + 2];
                uint32_t offset = ((uint32_t)bytecode[pc+3] << 24) | ((uint32_t)bytecode[pc+4] << 16) | 
                                  ((uint32_t)bytecode[pc+5] << 8) | bytecode[pc+6];
                
                if (src_ptr >= reg_count || dst >= reg_count) { free(regs); return QVM_ERR_INVALID_ARGUMENT; }
                
                if (!regs[src_ptr].is_valid || !regs[src_ptr].is_owned) {
                    printf("[VERIFIER] Error en PC=%u: Use-After-Free en lectura de r%u\n", pc, src_ptr);
                    free(regs);
                    return QVM_ERR_DOUBLE_FREE;
                }
                
                uint32_t access_size = 4;
                if (offset + access_size > regs[src_ptr].size) {
                    printf("[VERIFIER] Error en PC=%u: OUT OF BOUNDS! Offset=%u + Size=%u > Max=%u en r%u\n", 
                           pc, offset, access_size, regs[src_ptr].size, src_ptr);
                    free(regs);
                    return QVM_ERR_OUT_OF_BOUNDS;
                }
                
                pc += 8;
                break;
            }
            
            default:
                pc += 4;
                break;
        }
    }
    
    free(regs);
    return QVM_OK;
}
