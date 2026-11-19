/* QVM registers - acceso tipado a registros virtuales (QVM TS 5.6). */
#include "qvm/registers.h"

qvm_error qvm_register_read(const qvm_frame *frame, uint32_t index, qvm_value *out) {
    if (!frame || !out) return QVM_ERR_INVALID_ARGUMENT;
    if (index >= qvm_frame_register_count(frame)) return QVM_ERR_INVALID_ARGUMENT;
    const qvm_type_id_t *types = qvm_frame_register_types(frame);
    if (!types || types[index] == QVM_TYPE_VOID) return QVM_ERR_TYPE_MISMATCH;
    *out = qvm_frame_registers_const(frame)[index];
    return QVM_OK;
}

qvm_error qvm_register_write(qvm_frame *frame, uint32_t index, qvm_value v) {
    if (!frame) return QVM_ERR_INVALID_ARGUMENT;
    if (index >= qvm_frame_register_count(frame)) return QVM_ERR_INVALID_ARGUMENT;
    const qvm_type_id_t *types = qvm_frame_register_types(frame);
    if (!types || types[index] == QVM_TYPE_VOID)  return QVM_ERR_TYPE_MISMATCH;
    if (types[index] != v.type_id)                return QVM_ERR_TYPE_MISMATCH;
    qvm_frame_registers(frame)[index] = v;
    return QVM_OK;
}

qvm_error qvm_register_read_pair(const qvm_frame *frame, uint32_t index, uint64_t *lo, uint64_t *hi) {
    if (!frame || !lo || !hi) return QVM_ERR_INVALID_ARGUMENT;
    if (index % 2 != 0) return QVM_ERR_INVALID_ARGUMENT;
    if (index + 1 >= qvm_frame_register_count(frame)) return QVM_ERR_INVALID_ARGUMENT;
    const qvm_type_id_t *types = qvm_frame_register_types(frame);
    if (types[index] != QVM_TYPE_I128 && types[index] != QVM_TYPE_U128) return QVM_ERR_TYPE_MISMATCH;
    if (types[index] != types[index + 1]) return QVM_ERR_TYPE_MISMATCH;
    const qvm_value *regs = qvm_frame_registers_const(frame);
    *lo = regs[index].payload;
    *hi = regs[index + 1].payload;
    return QVM_OK;
}

qvm_error qvm_register_write_pair(qvm_frame *frame, uint32_t index, uint64_t lo, uint64_t hi) {
    if (!frame) return QVM_ERR_INVALID_ARGUMENT;
    if (index % 2 != 0) return QVM_ERR_INVALID_ARGUMENT;
    if (index + 1 >= qvm_frame_register_count(frame)) return QVM_ERR_INVALID_ARGUMENT;
    const qvm_type_id_t *types = qvm_frame_register_types(frame);
    if (types[index] != QVM_TYPE_I128 && types[index] != QVM_TYPE_U128) return QVM_ERR_TYPE_MISMATCH;
    if (types[index] != types[index + 1]) return QVM_ERR_TYPE_MISMATCH;
    qvm_value *regs = qvm_frame_registers(frame);
    regs[index].payload     = lo;
    regs[index + 1].payload = hi;
    return QVM_OK;
}
