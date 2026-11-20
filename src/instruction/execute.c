#include <string.h>
#include "qvm/instruction.h"
#include "qvm/registers.h"

static qvm_error exec_arith(const qvm_instruction *ins, qvm_frame *frame) {
    qvm_value a, b, result = {0};
    qvm_error err = qvm_register_read(frame, ins->src1, &a);
    if (err != QVM_OK) return err;
    err = qvm_register_read(frame, ins->src2, &b);
    if (err != QVM_OK) return err;
    result.type_id = ins->type_id;
    switch (ins->type_id) {
        case QVM_TYPE_I8: case QVM_TYPE_I16: case QVM_TYPE_I32: case QVM_TYPE_I64:
        case QVM_TYPE_U8: case QVM_TYPE_U16: case QVM_TYPE_U32: case QVM_TYPE_U64:
            switch (ins->opcode) {
                case QVM_OP_ARITH_ADD: result.payload = a.payload + b.payload; break;
                case QVM_OP_ARITH_SUB: result.payload = a.payload - b.payload; break;
                case QVM_OP_ARITH_MUL: result.payload = a.payload * b.payload; break;
                case QVM_OP_ARITH_DIV:
                    if (b.payload == 0) return QVM_ERR_INVALID_ARGUMENT;
                    result.payload = a.payload / b.payload; break;
                case QVM_OP_ARITH_MOD:
                    if (b.payload == 0) return QVM_ERR_INVALID_ARGUMENT;
                    result.payload = a.payload % b.payload; break;
                default: return QVM_ERR_INTERNAL;
            }
            break;
        case QVM_TYPE_F32: case QVM_TYPE_F64: {
            double va = (ins->type_id == QVM_TYPE_F32) ? qvm_value_as_f32(&a) : qvm_value_as_f64(&a);
            double vb = (ins->type_id == QVM_TYPE_F32) ? qvm_value_as_f32(&b) : qvm_value_as_f64(&b);
            double vr = 0;
            switch (ins->opcode) {
                case QVM_OP_ARITH_ADD: vr = va + vb; break;
                case QVM_OP_ARITH_SUB: vr = va - vb; break;
                case QVM_OP_ARITH_MUL: vr = va * vb; break;
                case QVM_OP_ARITH_DIV:
                    if (vb == 0) return QVM_ERR_INVALID_ARGUMENT;
                    vr = va / vb; break;
                default: return QVM_ERR_INTERNAL;
            }
            if (ins->type_id == QVM_TYPE_F32) {
                float f = (float)vr;
                memcpy(&result.payload, &f, sizeof(float));
            } else {
                memcpy(&result.payload, &vr, sizeof(double));
            }
            break;
        }
        default: return QVM_ERR_INTERNAL;
    }
    return qvm_register_write(frame, ins->dst, result);
}

static qvm_error exec_cmp(const qvm_instruction *ins, qvm_frame *frame) {
    qvm_value a, b, result = {0};
    qvm_error err = qvm_register_read(frame, ins->src1, &a);
    if (err != QVM_OK) return err;
    err = qvm_register_read(frame, ins->src2, &b);
    if (err != QVM_OK) return err;
    result.type_id = QVM_TYPE_BOOL;
    switch (ins->opcode) {
        case QVM_OP_CMP_EQ: result.payload = (a.payload == b.payload) ? 1 : 0; break;
        case QVM_OP_CMP_NE: result.payload = (a.payload != b.payload) ? 1 : 0; break;
        case QVM_OP_CMP_LT: result.payload = (a.payload < b.payload) ? 1 : 0; break;
        case QVM_OP_CMP_LE: result.payload = (a.payload <= b.payload) ? 1 : 0; break;
        case QVM_OP_CMP_GT: result.payload = (a.payload > b.payload) ? 1 : 0; break;
        case QVM_OP_CMP_GE: result.payload = (a.payload >= b.payload) ? 1 : 0; break;
        default: return QVM_ERR_INTERNAL;
    }
    return qvm_register_write(frame, ins->dst, result);
}

static qvm_error exec_logic(const qvm_instruction *ins, qvm_frame *frame) {
    qvm_value result = {0};
    result.type_id = QVM_TYPE_BOOL;
    if (ins->opcode == QVM_OP_LOGIC_NOT) {
        qvm_value a;
        qvm_error err = qvm_register_read(frame, ins->src1, &a);
        if (err != QVM_OK) return err;
        result.payload = (a.payload == 0) ? 1 : 0;
    } else {
        qvm_value a, b;
        qvm_error err = qvm_register_read(frame, ins->src1, &a);
        if (err != QVM_OK) return err;
        err = qvm_register_read(frame, ins->src2, &b);
        if (err != QVM_OK) return err;
        if (ins->opcode == QVM_OP_LOGIC_AND) {
            result.payload = (a.payload != 0 && b.payload != 0) ? 1 : 0;
        } else {
            result.payload = (a.payload != 0 || b.payload != 0) ? 1 : 0;
        }
    }
    return qvm_register_write(frame, ins->dst, result);
}

qvm_error qvm_instruction_execute(const qvm_instruction *ins, qvm_frame *frame) {
    if (!ins || !frame) return QVM_ERR_INVALID_ARGUMENT;
    switch (ins->opcode) {
        case QVM_OP_MOV: {
            qvm_value v;
            qvm_error err = qvm_register_read(frame, ins->src1, &v);
            if (err != QVM_OK) return err;
            return qvm_register_write(frame, ins->dst, v);
        }
        case QVM_OP_ARITH_ADD: case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL: case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
            return exec_arith(ins, frame);
        case QVM_OP_ARITH_NEG: {
            qvm_value a, result = {0};
            qvm_error err = qvm_register_read(frame, ins->src1, &a);
            if (err != QVM_OK) return err;
            result.type_id = ins->type_id;
            if (ins->type_id == QVM_TYPE_F32 || ins->type_id == QVM_TYPE_F64) {
                double v = (ins->type_id == QVM_TYPE_F32) ? qvm_value_as_f32(&a) : qvm_value_as_f64(&a);
                v = -v;
                if (ins->type_id == QVM_TYPE_F32) {
                    float f = (float)v;
                    memcpy(&result.payload, &f, sizeof(float));
                } else {
                    memcpy(&result.payload, &v, sizeof(double));
                }
            } else {
                result.payload = (uint64_t)(-(int64_t)a.payload);
            }
            return qvm_register_write(frame, ins->dst, result);
        }
        case QVM_OP_CMP_EQ: case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT: case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT: case QVM_OP_CMP_GE:
            return exec_cmp(ins, frame);
        case QVM_OP_LOGIC_AND: case QVM_OP_LOGIC_OR: case QVM_OP_LOGIC_NOT:
            return exec_logic(ins, frame);
        case QVM_OP_LOAD_CONST:
        case QVM_OP_JMP: case QVM_OP_JMP_IF: case QVM_OP_JMP_IF_NOT:
        case QVM_OP_CALL: case QVM_OP_RETURN: case QVM_OP_RETURN_VOID:
        case QVM_OP_CAST:
            return QVM_ERR_INTERNAL;
        default:
            return QVM_ERR_INTERNAL;
    }
}
