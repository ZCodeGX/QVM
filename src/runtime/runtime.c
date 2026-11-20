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
    rt->ctx    = ctx;
    rt->module = module;

    const qvm_limits *limits = qvm_context_limits(ctx);
    rt->max_call_depth = (limits && limits->max_call_depth) ? limits->max_call_depth : 256;

    rt->call_stack          = (qvm_frame **)calloc(rt->max_call_depth, sizeof(qvm_frame *));
    rt->return_addresses    = (uint32_t *)calloc(rt->max_call_depth, sizeof(uint32_t));
    rt->return_destinations = (uint32_t *)calloc(rt->max_call_depth, sizeof(uint32_t));
    if (!rt->call_stack || !rt->return_addresses || !rt->return_destinations) {
        free(rt->call_stack);
        free(rt->return_addresses);
        free(rt->return_destinations);
        free(rt);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    return rt;
}

void qvm_runtime_destroy(qvm_runtime *rt) {
    if (!rt) return;
    for (uint32_t i = 0; i < rt->call_depth; ++i) {
        qvm_frame_destroy(rt->call_stack[i]);
    }
    free(rt->call_stack);
    free(rt->return_addresses);
    free(rt->return_destinations);
    free(rt);
}

static uint32_t insn_size(const qvm_instruction *ins) {
    switch (ins->opcode) {
        case QVM_OP_RETURN_VOID: return 4;
        case QVM_OP_MOV: case QVM_OP_CAST:
        case QVM_OP_ARITH_NEG: case QVM_OP_LOGIC_NOT:
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
        case QVM_OP_LOAD_CONST: case QVM_OP_JMP:
            return 8;
        case QVM_OP_JMP_IF: case QVM_OP_JMP_IF_NOT:
            return 9;
        case QVM_OP_CALL:
            return 9u + ins->arg_count;
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
            &ins);
        if (err != QVM_OK) return err;

        qvm_frame *frame = rt->call_stack[rt->call_depth - 1];
        uint32_t   size  = insn_size(&ins);

        switch (ins.opcode) {
            case QVM_OP_LOAD_CONST: {
                if (ins.imm32 >= rt->module->constant_count) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_INVALID_BYTECODE;
                }
                err = qvm_register_write(frame, ins.dst, rt->module->constants[ins.imm32]);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                rt->pc += size;
                break;
            }
            case QVM_OP_JMP:
                if (ins.imm32 >= rt->module->bytecode_size) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_INVALID_BYTECODE;
                }
                rt->pc = ins.imm32;
                break;

            case QVM_OP_JMP_IF:
            case QVM_OP_JMP_IF_NOT: {
                qvm_value cond;
                err = qvm_register_read(frame, ins.src1, &cond);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                int take = (ins.opcode == QVM_OP_JMP_IF) ? (cond.payload != 0) : (cond.payload == 0);
                if (take) {
                    if (ins.imm32 >= rt->module->bytecode_size) {
                        qvm_instruction_free(&ins);
                        return QVM_ERR_INVALID_BYTECODE;
                    }
                    rt->pc = ins.imm32;
                } else {
                    rt->pc += size;
                }
                break;
            }
            case QVM_OP_CALL: {
                if (ins.imm32 >= rt->module->function_count) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_INVALID_BYTECODE;
                }
                if (rt->call_depth >= rt->max_call_depth) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_CALL_DEPTH_EXCEEDED;
                }
                
                qvm_function *func = &rt->module->functions[ins.imm32];
                if (ins.arg_count != func->param_count) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_INVALID_BYTECODE;
                }
                
                qvm_frame *new_frame = qvm_frame_create(
                    ins.imm32,
                    func->local_count,
                    func->local_types,
                    frame,
                    &err);
                if (!new_frame) {
                    qvm_instruction_free(&ins);
                    return err;
                }
                
                for (uint16_t i = 0; i < func->param_count; ++i) {
                    qvm_value arg;
                    err = qvm_register_read(frame, ins.args[i], &arg);
                    if (err != QVM_OK) {
                        qvm_frame_destroy(new_frame);
                        qvm_instruction_free(&ins);
                        return err;
                    }
                    err = qvm_register_write(new_frame, i, arg);
                    if (err != QVM_OK) {
                        qvm_frame_destroy(new_frame);
                        qvm_instruction_free(&ins);
                        return err;
                    }
                }
                
                rt->return_addresses[rt->call_depth]    = rt->pc + size;
                rt->return_destinations[rt->call_depth] = ins.dst;
                rt->call_stack[rt->call_depth] = new_frame;
                rt->call_depth++;
                rt->pc = func->bytecode_offset;
                break;
            }
            case QVM_OP_RETURN: {
                err = qvm_register_read(frame, ins.src1, &rt->return_value);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                rt->has_return = 1;
                qvm_instruction_free(&ins);
                
                if (rt->call_depth == 1) return QVM_OK;
                
                uint32_t return_pc  = rt->return_addresses[rt->call_depth - 1];
                uint32_t return_dst = rt->return_destinations[rt->call_depth - 1];
                qvm_frame_destroy(frame);
                rt->call_stack[--rt->call_depth] = NULL;
                
                qvm_frame *caller = rt->call_stack[rt->call_depth - 1];
                err = qvm_register_write(caller, return_dst, rt->return_value);
                if (err != QVM_OK) return err;
                
                rt->pc = return_pc;
                break;
            }
            case QVM_OP_RETURN_VOID:
                qvm_instruction_free(&ins);
                if (rt->call_depth == 1) return QVM_OK;
                
                uint32_t return_pc = rt->return_addresses[rt->call_depth - 1];
                qvm_frame_destroy(frame);
                rt->call_stack[--rt->call_depth] = NULL;
                rt->pc = return_pc;
                break;

            default:
                err = qvm_instruction_execute(&ins, frame);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                rt->pc += size;
                break;
        }
        qvm_instruction_free(&ins);
    }
    return QVM_OK;
}
