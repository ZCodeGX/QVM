#include <string.h>
#include <stdlib.h>
#include "qvm/instruction.h"

static uint32_t rd32_le(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static uint16_t rd16_le(const uint8_t *b) {
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

qvm_error qvm_instruction_decode(const uint8_t *bytecode, uint32_t size, qvm_instruction *ins) {
    if (!bytecode || !ins || size < 1) return QVM_ERR_INVALID_BYTECODE;
    
    memset(ins, 0, sizeof(qvm_instruction));
    ins->opcode = (qvm_opcode)bytecode[0];
    ins->dst = QVM_INSN_NO_REG;
    ins->src1 = QVM_INSN_NO_REG;
    ins->src2 = QVM_INSN_NO_REG;
    
    switch (ins->opcode) {
        case QVM_OP_RETURN_VOID:
            if (size < 4) return QVM_ERR_INVALID_BYTECODE;
            break;
            
        case QVM_OP_MOV:
        case QVM_OP_CAST:
        case QVM_OP_ARITH_NEG:
        case QVM_OP_LOGIC_NOT:
        case QVM_OP_RETURN:
        case QVM_OP_STR_LEN:
        case QVM_OP_FREE:
            if (size < 5) return QVM_ERR_INVALID_BYTECODE;
            ins->dst = bytecode[1];
            ins->type_id = rd16_le(bytecode + 2);
            ins->src1 = bytecode[4];
            break;
            
        case QVM_OP_ARITH_ADD: case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL: case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
        case QVM_OP_CMP_EQ: case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT: case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT: case QVM_OP_CMP_GE:
        case QVM_OP_LOGIC_AND: case QVM_OP_LOGIC_OR:
        case QVM_OP_STR_CONCAT: case QVM_OP_STR_CHAR_AT:
        case QVM_OP_STR_SLICE: case QVM_OP_STR_CMP:
            if (size < 6) return QVM_ERR_INVALID_BYTECODE;
            ins->dst = bytecode[1];
            ins->type_id = rd16_le(bytecode + 2);
            ins->src1 = bytecode[4];
            ins->src2 = bytecode[5];
            break;
            
        case QVM_OP_LOAD_CONST:
        case QVM_OP_JMP:
        case QVM_OP_ALLOC:
            if (size < 8) return QVM_ERR_INVALID_BYTECODE;
            ins->dst = bytecode[1];
            ins->type_id = rd16_le(bytecode + 2);
            ins->imm32 = rd32_le(bytecode + 4);
            break;
            
        case QVM_OP_JMP_IF:
        case QVM_OP_JMP_IF_NOT:
            if (size < 9) return QVM_ERR_INVALID_BYTECODE;
            ins->dst = bytecode[1];
            ins->type_id = rd16_le(bytecode + 2);
            ins->src1 = bytecode[4];
            ins->imm32 = rd32_le(bytecode + 5);
            break;
            
        case QVM_OP_CALL:
        case QVM_OP_CALL_NATIVE:
            if (size < 9) return QVM_ERR_INVALID_BYTECODE;
            ins->dst = bytecode[1];
            ins->type_id = rd16_le(bytecode + 2);
            ins->imm32 = rd32_le(bytecode + 4);
            ins->arg_count = bytecode[8];
            if (size < 9u + ins->arg_count) return QVM_ERR_INVALID_BYTECODE;
            for (uint8_t i = 0; i < ins->arg_count && i < 4; i++) {
                ins->args[i] = bytecode[9 + i];
            }
            break;
            
        // QMS-Zero: LOAD_PTR y STORE_PTR tienen 9 bytes
        // Formato: [opcode, dst, src, type_id(2), imm32(4)]
        case QVM_OP_LOAD_PTR:
        case QVM_OP_STORE_PTR:
            if (size < 9) return QVM_ERR_INVALID_BYTECODE;
            ins->dst = bytecode[1];
            ins->src1 = bytecode[2];
            ins->type_id = rd16_le(bytecode + 3);
            ins->imm32 = rd32_le(bytecode + 5);
            break;
            
        default:
            return QVM_ERR_INVALID_BYTECODE;
    }
    
    return QVM_OK;
}
