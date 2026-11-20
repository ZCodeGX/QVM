#include <stdio.h>
#include <string.h>
#include "qvm/verifier.h"
#include "qvm/instruction.h"

static uint32_t insn_size(const qvm_instruction *ins) {
    switch (ins->opcode) {
        case QVM_OP_RETURN_VOID: return 4;
        case QVM_OP_MOV: case QVM_OP_CAST:
        case QVM_OP_ARITH_NEG: case QVM_OP_LOGIC_NOT:
        case QVM_OP_RETURN:
        case QVM_OP_STR_LEN:
            return 5;
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
            return 6;
        case QVM_OP_LOAD_CONST: case QVM_OP_JMP:
            return 8;
        case QVM_OP_JMP_IF: case QVM_OP_JMP_IF_NOT:
            return 9;
        case QVM_OP_CALL:
        case QVM_OP_CALL_NATIVE:
            return 9u + ins->arg_count;
        default:
            return 4;
    }
}

static void set_result(qvm_verifier_result *result, qvm_error err, uint32_t pc, const char *msg) {
    if (!result) return;
    result->error = err;
    result->pc    = pc;
    if (msg) {
        strncpy(result->message, msg, sizeof(result->message) - 1);
        result->message[sizeof(result->message) - 1] = '\0';
    } else {
        result->message[0] = '\0';
    }
}

qvm_error qvm_verify_module(const qvm_module *module, const qvm_context *ctx, qvm_verifier_result *result) {
    if (!module || !ctx) {
        set_result(result, QVM_ERR_INVALID_ARGUMENT, 0, "null module or context");
        return QVM_ERR_INVALID_ARGUMENT;
    }
    if (!module->bytecode || module->bytecode_size == 0) {
        set_result(result, QVM_ERR_INVALID_BYTECODE, 0, "empty bytecode");
        return QVM_ERR_INVALID_BYTECODE;
    }

    const qvm_limits *limits = qvm_context_limits(ctx);
    uint32_t max_pc = module->bytecode_size;
    uint32_t pc = 0;

    while (pc < max_pc) {
        qvm_instruction ins = {0};
        qvm_error err = qvm_instruction_decode(module->bytecode + pc, max_pc - pc, &ins);
        if (err != QVM_OK) {
            set_result(result, err, pc, "invalid instruction encoding");
            return err;
        }

        uint32_t size = insn_size(&ins);
        if (pc + size > max_pc) {
            set_result(result, QVM_ERR_INVALID_BYTECODE, pc, "instruction extends beyond bytecode");
            qvm_instruction_free(&ins);
            return QVM_ERR_INVALID_BYTECODE;
        }

        if (ins.opcode == QVM_OP_LOAD_CONST) {
            if (ins.imm32 >= module->constant_count) {
                set_result(result, QVM_ERR_INVALID_BYTECODE, pc, "constant index out of range");
                qvm_instruction_free(&ins);
                return QVM_ERR_INVALID_BYTECODE;
            }
        }

        if (ins.opcode == QVM_OP_JMP || ins.opcode == QVM_OP_JMP_IF || ins.opcode == QVM_OP_JMP_IF_NOT) {
            if (ins.imm32 >= max_pc) {
                set_result(result, QVM_ERR_INVALID_BYTECODE, pc, "jump target out of range");
                qvm_instruction_free(&ins);
                return QVM_ERR_INVALID_BYTECODE;
            }
        }

        if (ins.opcode == QVM_OP_CALL) {
            if (ins.imm32 >= module->function_count) {
                set_result(result, QVM_ERR_INVALID_BYTECODE, pc, "function index out of range");
                qvm_instruction_free(&ins);
                return QVM_ERR_INVALID_BYTECODE;
            }
            if (module->functions && ins.arg_count != module->functions[ins.imm32].param_count) {
                set_result(result, QVM_ERR_INVALID_BYTECODE, pc, "argument count mismatch");
                qvm_instruction_free(&ins);
                return QVM_ERR_INVALID_BYTECODE;
            }
        }

        // CALL_NATIVE no se valida aquí porque las funciones nativas están en el FFI registry
        // La validación se hace en runtime.c cuando se ejecuta la instrucción

        if (limits && limits->max_bytecode_bytes > 0 && max_pc > limits->max_bytecode_bytes) {
            set_result(result, QVM_ERR_LIMIT_EXCEEDED, 0, "bytecode exceeds limit");
            qvm_instruction_free(&ins);
            return QVM_ERR_LIMIT_EXCEEDED;
        }

        pc += size;
        qvm_instruction_free(&ins);
    }

    set_result(result, QVM_OK, 0, "verification passed");
    return QVM_OK;
}
