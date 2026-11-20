#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/runtime.h"
#include "qvm/registers.h"
#include "qvm/instruction.h"

static inline qvm_value* reg_ptr(qvm_frame *f, uint8_t r) {
    return &qvm_frame_registers(f)[r];
}

qvm_error qvm_runtime_execute_fast(qvm_runtime *rt) {
    if (!rt || !rt->module) return QVM_ERR_INVALID_ARGUMENT;
    if (rt->call_depth == 0) return QVM_ERR_INTERNAL;

    qvm_frame *frame = rt->call_stack[rt->call_depth - 1];
    const uint8_t *bc = rt->module->bytecode;
    uint32_t pc = rt->pc;
    
    // Computed goto para threaded code (elimina overhead del switch)
    #ifdef __GNUC__
    void *dispatch_table[] = {
        [QVM_OP_ARITH_ADD] = &&LABEL_ADD,
        [QVM_OP_CMP_LT] = &&LABEL_CMP_LT,
        [QVM_OP_JMP_IF] = &&LABEL_JMP_IF,
        [QVM_OP_LOAD_PTR] = &&LABEL_LOAD_PTR,
        [QVM_OP_STORE_PTR] = &&LABEL_STORE_PTR,
        [QVM_OP_RETURN] = &&LABEL_RETURN
    };
    
    #define DISPATCH() goto *dispatch_table[bc[pc]]
    
    DISPATCH();
    
    LABEL_ADD: {
        uint8_t dst = bc[pc + 1];
        uint8_t src1 = bc[pc + 4];
        uint8_t src2 = bc[pc + 5];
        reg_ptr(frame, dst)->payload = reg_ptr(frame, src1)->payload + reg_ptr(frame, src2)->payload;
        pc += 6;
        DISPATCH();
    }
    
    LABEL_CMP_LT: {
        uint8_t dst = bc[pc + 1];
        uint8_t src1 = bc[pc + 4];
        uint8_t src2 = bc[pc + 5];
        reg_ptr(frame, dst)->payload = (reg_ptr(frame, src1)->payload < reg_ptr(frame, src2)->payload) ? 1 : 0;
        pc += 6;
        DISPATCH();
    }
    
    LABEL_JMP_IF: {
        uint8_t src = bc[pc + 4];
        uint32_t target = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
        if (reg_ptr(frame, src)->payload != 0) {
            pc = target;
        } else {
            pc += 9;
        }
        DISPATCH();
    }
    
    LABEL_LOAD_PTR: {
        uint8_t dst = bc[pc + 1];
        uint8_t src = bc[pc + 2];
        uint32_t offset = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
        
        qvm_ptr *ptr = (qvm_ptr *)&reg_ptr(frame, src)->payload;
        void *addr = rt->frame_region.base + ptr->offset + offset;
        reg_ptr(frame, dst)->payload = *(int32_t *)addr;
        pc += 9;
        DISPATCH();
    }
    
    LABEL_STORE_PTR: {
        uint8_t dst = bc[pc + 1];
        uint8_t src = bc[pc + 2];
        uint32_t offset = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
        
        qvm_ptr *ptr = (qvm_ptr *)&reg_ptr(frame, dst)->payload;
        void *addr = rt->frame_region.base + ptr->offset + offset;
        *(int32_t *)addr = (int32_t)reg_ptr(frame, src)->payload;
        pc += 9;
        DISPATCH();
    }
    
    LABEL_RETURN: {
        uint8_t src = bc[pc + 1];
        rt->return_value = *reg_ptr(frame, src);
        rt->has_return = 1;
        return QVM_OK;
    }
    
    #else
    // Fallback para compiladores sin computed goto
    while (1) {
        uint8_t op = bc[pc];
        switch (op) {
            case QVM_OP_ARITH_ADD: {
                uint8_t dst = bc[pc + 1];
                uint8_t src1 = bc[pc + 4];
                uint8_t src2 = bc[pc + 5];
                reg_ptr(frame, dst)->payload = reg_ptr(frame, src1)->payload + reg_ptr(frame, src2)->payload;
                pc += 6;
                break;
            }
            case QVM_OP_CMP_LT: {
                uint8_t dst = bc[pc + 1];
                uint8_t src1 = bc[pc + 4];
                uint8_t src2 = bc[pc + 5];
                reg_ptr(frame, dst)->payload = (reg_ptr(frame, src1)->payload < reg_ptr(frame, src2)->payload) ? 1 : 0;
                pc += 6;
                break;
            }
            case QVM_OP_JMP_IF: {
                uint8_t src = bc[pc + 4];
                uint32_t target = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
                if (reg_ptr(frame, src)->payload != 0) {
                    pc = target;
                } else {
                    pc += 9;
                }
                break;
            }
            case QVM_OP_LOAD_PTR: {
                uint8_t dst = bc[pc + 1];
                uint8_t src = bc[pc + 2];
                uint32_t offset = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
                qvm_ptr *ptr = (qvm_ptr *)&reg_ptr(frame, src)->payload;
                void *addr = rt->frame_region.base + ptr->offset + offset;
                reg_ptr(frame, dst)->payload = *(int32_t *)addr;
                pc += 9;
                break;
            }
            case QVM_OP_STORE_PTR: {
                uint8_t dst = bc[pc + 1];
                uint8_t src = bc[pc + 2];
                uint32_t offset = bc[pc + 5] | (bc[pc + 6] << 8) | (bc[pc + 7] << 16) | (bc[pc + 8] << 24);
                qvm_ptr *ptr = (qvm_ptr *)&reg_ptr(frame, dst)->payload;
                void *addr = rt->frame_region.base + ptr->offset + offset;
                *(int32_t *)addr = (int32_t)reg_ptr(frame, src)->payload;
                pc += 9;
                break;
            }
            case QVM_OP_RETURN: {
                uint8_t src = bc[pc + 1];
                rt->return_value = *reg_ptr(frame, src);
                rt->has_return = 1;
                return QVM_OK;
            }
            default:
                return QVM_ERR_INVALID_BYTECODE;
        }
    }
    #endif
}
