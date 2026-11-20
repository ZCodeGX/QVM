#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/runtime.h"
#include "qvm/instruction.h"
#include "qvm/registers.h"
#include "qvm/string.h"
#include "qvm/memory.h"

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
    
    rt->ffi = qvm_ffi_registry_create(err);
    if (!rt->ffi) {
        free(rt->call_stack);
        free(rt->return_addresses);
        free(rt->return_destinations);
        free(rt);
        return NULL;
    }
    
    // Inicializar región de frame para QMS-Zero
    rt->frame_region.base = (uint8_t *)malloc(1024 * 1024); // 1MB por defecto
    rt->frame_region.capacity = 1024 * 1024;
    rt->frame_region.offset = 0;
    rt->frame_region.id = QVM_REGION_FRAME;
    
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
    qvm_ffi_registry_destroy(rt->ffi);
    free(rt->frame_region.base);
    free(rt);
}

qvm_ffi_registry *qvm_runtime_ffi(qvm_runtime *rt) {
    return rt ? rt->ffi : NULL;
}

static uint32_t insn_size(const qvm_instruction *ins) {
    switch (ins->opcode) {
        case QVM_OP_RETURN_VOID: return 4;
        case QVM_OP_MOV: case QVM_OP_CAST:
        case QVM_OP_ARITH_NEG: case QVM_OP_LOGIC_NOT:
        case QVM_OP_RETURN:
        case QVM_OP_STR_LEN:
        case QVM_OP_FREE:
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
        case QVM_OP_ALLOC:
            return 8;
        case QVM_OP_LOAD_PTR:
        case QVM_OP_STORE_PTR:
            return 9;
        case QVM_OP_JMP_IF: case QVM_OP_JMP_IF_NOT:
            return 9;
        case QVM_OP_CALL:
        case QVM_OP_CALL_NATIVE:
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
            case QVM_OP_MOV: {
                qvm_value v;
                err = qvm_register_read(frame, ins.src1, &v);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                err = qvm_register_write(frame, ins.dst, v);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                rt->pc += size;
                break;
            }
            case QVM_OP_ARITH_ADD: {
                qvm_value a, b, res = {0};
                err = qvm_register_read(frame, ins.src1, &a);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                err = qvm_register_read(frame, ins.src2, &b);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                res.type_id = ins.type_id;
                res.payload = a.payload + b.payload;
                err = qvm_register_write(frame, ins.dst, res);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                rt->pc += size;
                break;
            }
            case QVM_OP_ARITH_SUB: {
                qvm_value a, b, res = {0};
                err = qvm_register_read(frame, ins.src1, &a);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                err = qvm_register_read(frame, ins.src2, &b);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                res.type_id = ins.type_id;
                res.payload = a.payload - b.payload;
                err = qvm_register_write(frame, ins.dst, res);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                rt->pc += size;
                break;
            }
            case QVM_OP_CMP_LT: {
                qvm_value a, b, res = {0};
                err = qvm_register_read(frame, ins.src1, &a);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                err = qvm_register_read(frame, ins.src2, &b);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                res.type_id = QVM_TYPE_BOOL;
                res.payload = (a.payload < b.payload) ? 1 : 0;
                err = qvm_register_write(frame, ins.dst, res);
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

            case QVM_OP_JMP_IF: {
                qvm_value cond;
                err = qvm_register_read(frame, ins.src1, &cond);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                if (cond.payload != 0) {
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
            
            // ==========================================
            // QMS-Zero Opcodes (Zero-Overhead)
            // ==========================================
            
            case QVM_OP_ALLOC: {
                // ALLOC dst_reg, size, type_id
                // Sin checks: el Verifier ya validó todo
                uint32_t alloc_size = ins.imm32;
                uint16_t type_id = ins.type_id;
                
                // Bump allocation en la región de frame
                uint32_t offset = rt->frame_region.offset;
                rt->frame_region.offset += alloc_size;
                
                // Crear puntero QMS-Zero
                qvm_ptr ptr = {
                    .region_id = QVM_REGION_FRAME,
                    .offset = offset,
                    .size = alloc_size,
                    .type_id = type_id
                };
                
                // Guardar el puntero en el registro (como valor)
                qvm_value v = {0};
                v.type_id = QVM_TYPE_PTR;
                memcpy(&v.payload, &ptr, sizeof(qvm_ptr));
                
                err = qvm_register_write(frame, ins.dst, v);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                
                rt->pc += size;
                break;
            }
            
            case QVM_OP_FREE: {
                // FREE src_reg
                // Sin checks: el Verifier ya validó que es OWNED
                // En QMS-Zero con regiones, FREE es un no-op (la región se limpia sola)
                // Pero marcamos el registro como consumido para el Verifier
                
                qvm_value v = {0};
                v.type_id = QVM_TYPE_PTR;
                // Marcar como consumido (offset = 0, size = 0)
                qvm_ptr ptr = {0};
                memcpy(&v.payload, &ptr, sizeof(qvm_ptr));
                
                err = qvm_register_write(frame, ins.src1, v);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                
                rt->pc += size;
                break;
            }
            
            case QVM_OP_LOAD_PTR: {
                // LOAD_PTR dst_reg, src_ptr_reg, offset
                // ZERO-OVERHEAD: sin bounds checking, sin verificación de estado
                // El Verifier ya garantizó que es seguro
                
                qvm_value ptr_val;
                err = qvm_register_read(frame, ins.src1, &ptr_val);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                
                qvm_ptr ptr;
                memcpy(&ptr, &ptr_val.payload, sizeof(qvm_ptr));
                
                // Calcular dirección real: base + offset + access_offset
                uint32_t access_offset = ins.imm32;
                void *src_addr = rt->frame_region.base + ptr.offset + access_offset;
                
                // Leer 4 bytes (I32) directamente - SIN CHECKS
                int32_t value = *(int32_t *)src_addr;
                
                // Escribir en el registro destino
                qvm_value result = {0};
                result.type_id = QVM_TYPE_I32;
                result.payload = (uint64_t)value;
                
                err = qvm_register_write(frame, ins.dst, result);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                
                rt->pc += size;
                break;
            }
            
            case QVM_OP_STORE_PTR: {
                // STORE_PTR dst_ptr_reg, src_reg, offset
                // ZERO-OVERHEAD: sin bounds checking, sin verificación de estado
                
                qvm_value ptr_val;
                err = qvm_register_read(frame, ins.dst, &ptr_val);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                
                qvm_ptr ptr;
                memcpy(&ptr, &ptr_val.payload, sizeof(qvm_ptr));
                
                qvm_value src_val;
                err = qvm_register_read(frame, ins.src1, &src_val);
                if (err != QVM_OK) { qvm_instruction_free(&ins); return err; }
                
                // Calcular dirección real
                uint32_t access_offset = ins.imm32;
                void *dst_addr = rt->frame_region.base + ptr.offset + access_offset;
                
                // Escribir 4 bytes (I32) directamente - SIN CHECKS
                *(int32_t *)dst_addr = (int32_t)src_val.payload;
                
                rt->pc += size;
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
            case QVM_OP_CALL_NATIVE: {
                if (!rt->ffi || ins.imm32 >= rt->ffi->count) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_INVALID_BYTECODE;
                }
                const qvm_native_function *native = &rt->ffi->functions[ins.imm32];
                if (ins.arg_count != native->arg_count) {
                    qvm_instruction_free(&ins);
                    return QVM_ERR_INVALID_ARGUMENT;
                }
                
                qvm_value *args = NULL;
                if (ins.arg_count > 0) {
                    args = (qvm_value *)malloc(ins.arg_count * sizeof(qvm_value));
                    if (!args) {
                        qvm_instruction_free(&ins);
                        return QVM_ERR_OUT_OF_MEMORY;
                    }
                    for (uint16_t i = 0; i < ins.arg_count; ++i) {
                        err = qvm_register_read(frame, ins.args[i], &args[i]);
                        if (err != QVM_OK) {
                            free(args);
                            qvm_instruction_free(&ins);
                            return err;
                        }
                    }
                }
                
                qvm_value result = {0};
                err = native->fn(frame, args, ins.arg_count, &result);
                free(args);
                
                if (err != QVM_OK) {
                    qvm_instruction_free(&ins);
                    return err;
                }
                
                if (native->return_type != QVM_TYPE_VOID && ins.dst != QVM_INSN_NO_REG) {
                    err = qvm_register_write(frame, ins.dst, result);
                    if (err != QVM_OK) {
                        qvm_instruction_free(&ins);
                        return err;
                    }
                }
                
                rt->pc += size;
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
