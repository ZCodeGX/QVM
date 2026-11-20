#include "qvm/types.h"

int qvm_type_is_native(qvm_type_id_t id) {
    return id >= QVM_TYPE_ID_MIN_NATIVE && id <= QVM_TYPE_ID_MAX_NATIVE;
}

int qvm_type_is_extended(qvm_type_id_t id) {
    return id >= QVM_TYPE_ID_MIN_EXTENDED && id <= QVM_TYPE_ID_MAX_EXTENDED;
}

int qvm_type_is_valid(qvm_type_id_t id) {
    if (id == QVM_TYPE_VOID) return 1;
    return qvm_type_is_native(id) || qvm_type_is_extended(id);
}

size_t qvm_type_slot_count(qvm_type_id_t id) {
    switch (id) {
        case QVM_TYPE_VOID:   return 0;
        case QVM_TYPE_BOOL:
        case QVM_TYPE_I8:     case QVM_TYPE_U8:
        case QVM_TYPE_I16:    case QVM_TYPE_U16:
        case QVM_TYPE_I32:    case QVM_TYPE_U32:
        case QVM_TYPE_F32:
        case QVM_TYPE_CHAR:
        case QVM_TYPE_REF:    case QVM_TYPE_PTR:
            return 1;
        case QVM_TYPE_I64:    case QVM_TYPE_U64:
        case QVM_TYPE_F64:
        case QVM_TYPE_STRING:
            return 2;
        case QVM_TYPE_I128:   case QVM_TYPE_U128:
            return 2;
        default:
            return 1;
    }
}

size_t qvm_type_native_size(qvm_type_id_t id) {
    switch (id) {
        case QVM_TYPE_VOID:   return 0;
        case QVM_TYPE_BOOL:
        case QVM_TYPE_I8:     case QVM_TYPE_U8:
            return 1;
        case QVM_TYPE_I16:    case QVM_TYPE_U16:
            return 2;
        case QVM_TYPE_I32:    case QVM_TYPE_U32:
        case QVM_TYPE_F32:
        case QVM_TYPE_CHAR:
            return 4;
        case QVM_TYPE_I64:    case QVM_TYPE_U64:
        case QVM_TYPE_F64:
        case QVM_TYPE_REF:    case QVM_TYPE_PTR:
            return 8;
        case QVM_TYPE_I128:   case QVM_TYPE_U128:
            return 16;
        case QVM_TYPE_STRING:
            return 16;
        default:
            return 0;
    }
}
