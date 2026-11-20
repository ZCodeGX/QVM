#include <stdlib.h>
#include "qvm/runtime.h"
#include "qvm/registers.h"

qvm_runtime *qvm_runtime_create(qvm_context *ctx, qvm_module *module, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!ctx || !module) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    qvm_runtime *rt = (qvm_runtime *)calloc(1, sizeof *rt);
    if (!rt) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    rt->ctx = ctx;
    rt->module = module;
    
    const qvm_limits *limits = qvm_context_limits(ctx);
    rt->max_call_depth = limits ? limits->max_call_depth : 256;
    
    rt->call_stack = (qvm_frame **)calloc(rt->max_call_depth, sizeof(qvm_frame *));
    if (!rt->call_stack) {
        free(rt);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    rt->call_depth = 0;
    rt->pc = 0;
    return rt;
}

void qvm_runtime_destroy(qvm_runtime *rt) {
    if (!rt) return;
    for (uint32_t i = 0; i < rt->call_depth; ++i) {
        qvm_frame_destroy(rt->call_stack[i]);
    }
    free(rt->call_stack);
    free(rt);
}

static uint32_t get_instruction_size(const qvm_instruction *ins) {
    switch (ins->opcode) {
        case QVM_OP_RETURN_VOID: return 4;
        case QVM_OP_MOV:
        case QVM_OP_CAST:
        case QVM_OP_ARITH_NEG:
        case QVM_OP_LOGIC_NOT:
        case QVM_OP_RETURN:
            return 5;
        case QVM_OP_ARITH_ADD: case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL: case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
        case QVM_OP_CMP_EQ: case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT: case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT: case QVM_OP_CMP_GE:
        case QVM_OP_LOGIC_AND: case QVM_OP_LOGIC_OR:
            return 6;
        case QVM_OP_LOAD_CONST:
        case QVM_OP_JMP:
            return 8;
        case QVM_OP_JMP_IF:
        case QVM_OP_JMP_IF_NOT:
            return 9;
        case QVM_OP_CALL:
            return 9 + ins->arg_count;
        default:
            return 4;
    }
}

qvm_error qvm_runtime_execute(qvm_runtime *rt) {
    if (!rt || !rt->module) return QVM_ERR_INVALID_ARGUMENT;
    if (rt->call_depth == 0) return QVM_ERR_INTERNAL;
    
    while (rt->pc < rt->module->bytecode_size) {
        qvm_instruction ins = {0};
        qvm_error err = qvm_instruction_decode(
            rt->module->bytecode + rt->pc,
            rt->module->bytecode_size - rt->pc,
            &ins
        );
        if (err != QVM_OK) return err;
        
        qvm_frame *frame = rt->call_stack[rt->call_depth - 1];
        
        switch (ins.opcode) {
            case QVM_OP_JMP:
                rt->pc = ins.imm32;
                break;
            case QVM_OP_JMP_IF: {
                qvm_value cond;
                err = qvm_register_read(frame, ins.src1, &cond);
                if (err != QVM_OK) return err;
                if (cond.payload != 0) rt->pc = ins.imm32;
                else rt->pc += get_instruction_size(&ins);
                break;
            }
            case QVM_OP_JMP_IF_NOT: {
                qvm_value cond;
                err = qvm_register_read(frame, ins.src1, &cond);
                if (err != QVM_OK) return err;
                if (cond.payload == 0) rt->pc = ins.imm32;
                else rt->pc += get_instruction_size(&ins);
                break;
            }
            case QVM_OP_RETURN:
            case QVM_OP_RETURN_VOID:
                if (rt->call_depth == 1) return QVM_OK;
                qvm_frame_destroy(frame);
                rt->call_depth--;
                return QVM_OK;
            default:
                err = qvm_instruction_execute(&ins, frame);
                if (err != QVM_OK) return err;
                rt->pc += get_instruction_size(&ins);
                break;
        }
    }
    return QVM_OK;
}
