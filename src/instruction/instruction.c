#include <string.h>
#include <stdlib.h>
#include "qvm/instruction.h"
#include "qvm/frames.h"

const char* qvm_opcode_name(qvm_opcode op) {
    switch (op) {
        case QVM_OP_JMP: return "JMP";
        case QVM_OP_JMP_IF: return "JMP_IF";
        case QVM_OP_JMP_IF_NOT: return "JMP_IF_NOT";
        case QVM_OP_CALL: return "CALL";
        case QVM_OP_CALL_NATIVE: return "CALL_NATIVE";
        case QVM_OP_RETURN: return "RETURN";
        case QVM_OP_RETURN_VOID: return "RETURN_VOID";
        case QVM_OP_MOV: return "MOV";
        case QVM_OP_LOAD_CONST: return "LOAD_CONST";
        case QVM_OP_ARITH_ADD: return "ARITH_ADD";
        case QVM_OP_ARITH_SUB: return "ARITH_SUB";
        case QVM_OP_ARITH_MUL: return "ARITH_MUL";
        case QVM_OP_ARITH_DIV: return "ARITH_DIV";
        case QVM_OP_ARITH_MOD: return "ARITH_MOD";
        case QVM_OP_ARITH_NEG: return "ARITH_NEG";
        case QVM_OP_CMP_EQ: return "CMP_EQ";
        case QVM_OP_CMP_NE: return "CMP_NE";
        case QVM_OP_CMP_LT: return "CMP_LT";
        case QVM_OP_CMP_LE: return "CMP_LE";
        case QVM_OP_CMP_GT: return "CMP_GT";
        case QVM_OP_CMP_GE: return "CMP_GE";
        case QVM_OP_LOGIC_AND: return "LOGIC_AND";
        case QVM_OP_LOGIC_OR: return "LOGIC_OR";
        case QVM_OP_LOGIC_NOT: return "LOGIC_NOT";
        case QVM_OP_CAST: return "CAST";
        case QVM_OP_STR_CONCAT: return "STR_CONCAT";
        case QVM_OP_STR_LEN: return "STR_LEN";
        case QVM_OP_STR_CHAR_AT: return "STR_CHAR_AT";
        case QVM_OP_STR_SLICE: return "STR_SLICE";
        case QVM_OP_STR_CMP: return "STR_CMP";
        case QVM_OP_ALLOC: return "ALLOC";
        case QVM_OP_FREE: return "FREE";
        case QVM_OP_LOAD_PTR: return "LOAD_PTR";
        case QVM_OP_STORE_PTR: return "STORE_PTR";
        default: return "UNKNOWN";
    }
}

void qvm_instruction_free(qvm_instruction *ins) {
    if (!ins) return;
    // Los args ahora están en el array fijo, no hay nada que liberar
    // Pero el test espera que arg_count se ponga a 0
    ins->arg_count = 0;
}

qvm_type_id_t qvm_instruction_result_type(const qvm_instruction *ins) {
    if (!ins) return QVM_TYPE_VOID;
    
    switch (ins->opcode) {
        case QVM_OP_CMP_EQ:
        case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT:
        case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT:
        case QVM_OP_CMP_GE:
        case QVM_OP_LOGIC_AND:
        case QVM_OP_LOGIC_OR:
        case QVM_OP_LOGIC_NOT:
            return QVM_TYPE_BOOL;
        
        case QVM_OP_ARITH_ADD:
        case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL:
        case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
        case QVM_OP_ARITH_NEG:
            return ins->type_id;
        
        case QVM_OP_CAST:
            return ins->type_id;
        
        case QVM_OP_STR_LEN:
            return QVM_TYPE_I32;
        
        case QVM_OP_STR_CONCAT:
        case QVM_OP_STR_CHAR_AT:
        case QVM_OP_STR_SLICE:
            return QVM_TYPE_STRING;
        
        default:
            return QVM_TYPE_VOID;
    }
}

qvm_error qvm_instruction_validate(const qvm_instruction *ins, struct qvm_frame *frame) {
    if (!ins || !frame) return QVM_ERR_INVALID_ARGUMENT;
    
    uint32_t reg_count = qvm_frame_register_count(frame);
    
    if (ins->dst != QVM_INSN_NO_REG && ins->dst >= reg_count) {
        return QVM_ERR_INVALID_ARGUMENT;
    }
    
    if (ins->src1 != QVM_INSN_NO_REG && ins->src1 >= reg_count) {
        return QVM_ERR_INVALID_ARGUMENT;
    }
    
    if (ins->src2 != QVM_INSN_NO_REG && ins->src2 >= reg_count) {
        return QVM_ERR_INVALID_ARGUMENT;
    }
    
    if (ins->opcode == QVM_OP_CALL || ins->opcode == QVM_OP_CALL_NATIVE) {
        for (uint8_t i = 0; i < ins->arg_count; i++) {
            if (ins->args[i] >= reg_count) {
                return QVM_ERR_INVALID_ARGUMENT;
            }
        }
    }
    
    return QVM_OK;
}
