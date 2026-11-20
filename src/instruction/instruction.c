#include <stddef.h>
#include "qvm/instruction.h"

static const char *const opcode_names[] = {
    [QVM_OP_MOV]         = "MOV",
    [QVM_OP_LOAD_CONST]  = "LOAD_CONST",
    [QVM_OP_ARITH_ADD]   = "ARITH_ADD",
    [QVM_OP_ARITH_SUB]   = "ARITH_SUB",
    [QVM_OP_ARITH_MUL]   = "ARITH_MUL",
    [QVM_OP_ARITH_DIV]   = "ARITH_DIV",
    [QVM_OP_ARITH_MOD]   = "ARITH_MOD",
    [QVM_OP_ARITH_NEG]   = "ARITH_NEG",
    [QVM_OP_CMP_EQ]      = "CMP_EQ",
    [QVM_OP_CMP_NE]      = "CMP_NE",
    [QVM_OP_CMP_LT]      = "CMP_LT",
    [QVM_OP_CMP_LE]      = "CMP_LE",
    [QVM_OP_CMP_GT]      = "CMP_GT",
    [QVM_OP_CMP_GE]      = "CMP_GE",
    [QVM_OP_LOGIC_AND]   = "LOGIC_AND",
    [QVM_OP_LOGIC_OR]    = "LOGIC_OR",
    [QVM_OP_LOGIC_NOT]   = "LOGIC_NOT",
    [QVM_OP_JMP]         = "JMP",
    [QVM_OP_JMP_IF]      = "JMP_IF",
    [QVM_OP_JMP_IF_NOT]  = "JMP_IF_NOT",
    [QVM_OP_CALL]        = "CALL",
    [QVM_OP_RETURN]      = "RETURN",
    [QVM_OP_RETURN_VOID] = "RETURN_VOID",
    [QVM_OP_CAST]        = "CAST"
};

const char *qvm_opcode_name(qvm_opcode op) {
    size_t i = (size_t)op;
    if (i >= sizeof opcode_names / sizeof opcode_names[0]) return NULL;
    return opcode_names[i];
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

        case QVM_OP_JMP:
        case QVM_OP_JMP_IF:
        case QVM_OP_JMP_IF_NOT:
        case QVM_OP_RETURN:
        case QVM_OP_RETURN_VOID:
            return QVM_TYPE_VOID;

        default:
            return ins->type_id;
    }
}

static int check_reg(const qvm_frame *frame, uint32_t idx, int required) {
    if (idx == QVM_INSN_NO_REG) return !required;
    if (idx >= qvm_frame_register_count(frame)) return 0;

    const qvm_type_id_t *types = qvm_frame_register_types(frame);
    return types && types[idx] != QVM_TYPE_VOID;
}

static qvm_error validate_shape(const qvm_instruction *ins, const qvm_frame *frame) {
    switch (ins->opcode) {
        case QVM_OP_MOV:
        case QVM_OP_LOAD_CONST:
        case QVM_OP_ARITH_NEG:
        case QVM_OP_LOGIC_NOT:
        case QVM_OP_CAST:
            if (!check_reg(frame, ins->dst, 1)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 1)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 0)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        case QVM_OP_ARITH_ADD:
        case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL:
        case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
        case QVM_OP_CMP_EQ:
        case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT:
        case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT:
        case QVM_OP_CMP_GE:
        case QVM_OP_LOGIC_AND:
        case QVM_OP_LOGIC_OR:
            if (!check_reg(frame, ins->dst, 1)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 1)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 1)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        case QVM_OP_JMP:
            if (!check_reg(frame, ins->dst, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 0)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        case QVM_OP_JMP_IF:
        case QVM_OP_JMP_IF_NOT:
            if (!check_reg(frame, ins->dst, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 1)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 0)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        case QVM_OP_CALL:
            if (!check_reg(frame, ins->dst, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 0)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        case QVM_OP_RETURN:
            if (!check_reg(frame, ins->dst, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 1)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 0)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        case QVM_OP_RETURN_VOID:
            if (!check_reg(frame, ins->dst, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src1, 0)) return QVM_ERR_INVALID_ARGUMENT;
            if (!check_reg(frame, ins->src2, 0)) return QVM_ERR_INVALID_ARGUMENT;
            break;

        default:
            return QVM_ERR_INVALID_BYTECODE;
    }

    if (ins->opcode != QVM_OP_CALL && ins->arg_count != 0) {
        return QVM_ERR_INVALID_ARGUMENT;
    }

    return QVM_OK;
}

qvm_error qvm_instruction_validate(const qvm_instruction *ins, const qvm_frame *frame) {
    if (!ins || !frame) return QVM_ERR_INVALID_ARGUMENT;
    if (!qvm_opcode_name(ins->opcode)) return QVM_ERR_INVALID_BYTECODE;

    qvm_error err = validate_shape(ins, frame);
    if (err != QVM_OK) return err;

    if (ins->arg_count != 0 && !ins->args) {
        return QVM_ERR_INVALID_ARGUMENT;
    }

    for (uint16_t i = 0; i < ins->arg_count; ++i) {
        if (!check_reg(frame, ins->args[i], 1)) {
            return QVM_ERR_INVALID_ARGUMENT;
        }
    }

    return QVM_OK;
}
