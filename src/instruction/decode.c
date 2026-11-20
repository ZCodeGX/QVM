#include <stdlib.h>
#include "qvm/instruction.h"

static uint16_t rd16(const uint8_t *b) { return (uint16_t)(b[0] | (b[1] << 8)); }
static uint32_t rd32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static void init_out(qvm_instruction *out, uint8_t op, uint8_t dst, uint16_t tid) {
    out->opcode    = (qvm_opcode)op;
    out->type_id   = tid;
    out->dst       = (dst == 0xFF) ? QVM_INSN_NO_REG : (uint32_t)dst;
    out->src1      = QVM_INSN_NO_REG;
    out->src2      = QVM_INSN_NO_REG;
    out->imm32     = 0;
    out->args      = NULL;
    out->arg_count = 0;
}

static qvm_error decode_1src(qvm_instruction *out, const uint8_t *b, uint32_t len) {
    if (len < 5) return QVM_ERR_INVALID_BYTECODE;
    out->src1 = b[4];
    return QVM_OK;
}

static qvm_error decode_2src(qvm_instruction *out, const uint8_t *b, uint32_t len) {
    if (len < 6) return QVM_ERR_INVALID_BYTECODE;
    out->src1 = b[4];
    out->src2 = b[5];
    return QVM_OK;
}

static qvm_error decode_imm32(qvm_instruction *out, const uint8_t *b, uint32_t len) {
    if (len < 8) return QVM_ERR_INVALID_BYTECODE;
    out->imm32 = rd32(b + 4);
    return QVM_OK;
}

static qvm_error decode_jmp_cond(qvm_instruction *out, const uint8_t *b, uint32_t len) {
    if (len < 9) return QVM_ERR_INVALID_BYTECODE;
    out->src1  = b[4];
    out->imm32 = rd32(b + 5);
    return QVM_OK;
}

static qvm_error decode_call(qvm_instruction *out, const uint8_t *b, uint32_t len) {
    if (len < 9) return QVM_ERR_INVALID_BYTECODE;
    out->imm32     = rd32(b + 4);
    out->arg_count = b[8];
    if (len < 9u + out->arg_count) return QVM_ERR_INVALID_BYTECODE;
    if (out->arg_count > 0) {
        uint32_t *args = (uint32_t *)malloc(out->arg_count * sizeof(uint32_t));
        if (!args) return QVM_ERR_OUT_OF_MEMORY;
        for (uint16_t i = 0; i < out->arg_count; ++i) args[i] = b[9 + i];
        out->args = args;
    }
    return QVM_OK;
}

qvm_error qvm_instruction_decode(const uint8_t *bytes, uint32_t len, qvm_instruction *out) {
    if (!bytes || !out || len < 4) return QVM_ERR_INVALID_BYTECODE;

    init_out(out, bytes[0], bytes[1], rd16(bytes + 2));
    if (!qvm_opcode_name(out->opcode)) return QVM_ERR_INVALID_BYTECODE;

    switch (out->opcode) {
        case QVM_OP_RETURN_VOID:
            return QVM_OK;

        case QVM_OP_RETURN:
        case QVM_OP_MOV:
        case QVM_OP_CAST:
        case QVM_OP_ARITH_NEG:
        case QVM_OP_LOGIC_NOT:
        case QVM_OP_STR_LEN:
            return decode_1src(out, bytes, len);

        case QVM_OP_ARITH_ADD: case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL: case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
        case QVM_OP_CMP_EQ: case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT: case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT: case QVM_OP_CMP_GE:
        case QVM_OP_LOGIC_AND: case QVM_OP_LOGIC_OR:
        case QVM_OP_STR_CONCAT:
        case QVM_OP_STR_CHAR_AT:
        case QVM_OP_STR_SLICE:
        case QVM_OP_STR_CMP:
            return decode_2src(out, bytes, len);

        case QVM_OP_LOAD_CONST:
        case QVM_OP_JMP:
            return decode_imm32(out, bytes, len);

        case QVM_OP_JMP_IF:
        case QVM_OP_JMP_IF_NOT:
            return decode_jmp_cond(out, bytes, len);

        case QVM_OP_CALL:
        case QVM_OP_CALL_NATIVE:
            return decode_call(out, bytes, len);

        default:
            return QVM_ERR_INVALID_BYTECODE;
    }
}

void qvm_instruction_free(qvm_instruction *ins) {
    if (!ins) return;
    free(ins->args);
    ins->args      = NULL;
    ins->arg_count = 0;
}
