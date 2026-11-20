#include "qvm/instruction.h"
#include "qvm/registers.h"

static qvm_error exec_arith(const qvm_instruction *ins, qvm_frame *frame) {
    qvm_value a, b, result = {0};
    qvm_error err;
    
    err = qvm_register_read(frame, ins->src1, &a);
    if (err != QVM_OK) return err;
    err = qvm_register_read(frame, ins->src2, &b);
    if (err != QVM_OK) return err;
    
    result.type_id = ins->type_id;
    
    switch (ins->type_id) {
        case QVM_TYPE_I32: {
            int32_t va = (int32_t)a.payload, vb = (int32_t)b.payload;
            switch (ins->opcode) {
                case QVM_OP_ARITH_ADD: result.payload = (uint64_t)(va + vb); break;
                case QVM_OP_ARITH_SUB: result.payload = (uint64_t)(va - vb); break;
                case QVM_OP_ARITH_MUL: result.payload = (uint64_t)(va * vb); break;
                case QVM_OP_ARITH_DIV: 
                    if (vb == 0) return QVM_ERR_INVALID_ARGUMENT;
                    result.payload = (uint64_t)(va / vb); 
                    break;
                case QVM_OP_ARITH_MOD: 
                    if (vb == 0) return QVM_ERR_INVALID_ARGUMENT;
                    result.payload = (uint64_t)(va % vb); 
                    break;
                default: return QVM_ERR_INTERNAL;
            }
            break;
        }
        case QVM_TYPE_I64: {
            int64_t va = (int64_t)a.payload, vb = (int64_t)b.payload;
            switch (ins->opcode) {
                case QVM_OP_ARITH_ADD: result.payload = (uint64_t)(va + vb); break;
                case QVM_OP_ARITH_SUB: result.payload = (uint64_t)(va - vb); break;
                case QVM_OP_ARITH_MUL: result.payload = (uint64_t)(va * vb); break;
                case QVM_OP_ARITH_DIV: 
                    if (vb == 0) return QVM_ERR_INVALID_ARGUMENT;
                    result.payload = (uint64_t)(va / vb); 
                    break;
                case QVM_OP_ARITH_MOD: 
                    if (vb == 0) return QVM_ERR_INVALID_ARGUMENT;
                    result.payload = (uint64_t)(va % vb); 
                    break;
                default: return QVM_ERR_INTERNAL;
            }
            break;
        }
        default:
            return QVM_ERR_INTERNAL;
    }
    
    return qvm_register_write(frame, ins->dst, result);
}

static qvm_error exec_cmp(const qvm_instruction *ins, qvm_frame *frame) {
    qvm_value a, b, result = {0};
    qvm_error err;
    
    err = qvm_register_read(frame, ins->src1, &a);
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

qvm_error qvm_instruction_execute(const qvm_instruction *ins, qvm_frame *frame) {
    if (!ins || !frame) return QVM_ERR_INVALID_ARGUMENT;
    
    switch (ins->opcode) {
        case QVM_OP_MOV: {
            qvm_value v;
            qvm_error err = qvm_register_read(frame, ins->src1, &v);
            if (err != QVM_OK) return err;
            return qvm_register_write(frame, ins->dst, v);
        }
        case QVM_OP_ARITH_ADD:
        case QVM_OP_ARITH_SUB:
        case QVM_OP_ARITH_MUL:
        case QVM_OP_ARITH_DIV:
        case QVM_OP_ARITH_MOD:
            return exec_arith(ins, frame);
        case QVM_OP_CMP_EQ:
        case QVM_OP_CMP_NE:
        case QVM_OP_CMP_LT:
        case QVM_OP_CMP_LE:
        case QVM_OP_CMP_GT:
        case QVM_OP_CMP_GE:
            return exec_cmp(ins, frame);
        default:
            return QVM_ERR_INTERNAL;
    }
}
