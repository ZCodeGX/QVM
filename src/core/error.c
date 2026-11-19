#include "qvm/error.h"

const char *qvm_error_name(qvm_error err) {
    switch (err) {
        case QVM_OK:                      return "QVM_OK";
        case QVM_ERR_INVALID_ARGUMENT:    return "QVM_ERR_INVALID_ARGUMENT";
        case QVM_ERR_OUT_OF_MEMORY:       return "QVM_ERR_OUT_OF_MEMORY";
        case QVM_ERR_INVALID_BYTECODE:    return "QVM_ERR_INVALID_BYTECODE";
        case QVM_ERR_VERIFICATION_FAILED: return "QVM_ERR_VERIFICATION_FAILED";
        case QVM_ERR_TYPE_MISMATCH:       return "QVM_ERR_TYPE_MISMATCH";
        case QVM_ERR_STACK_OVERFLOW:      return "QVM_ERR_STACK_OVERFLOW";
        case QVM_ERR_CALL_DEPTH_EXCEEDED: return "QVM_ERR_CALL_DEPTH_EXCEEDED";
        case QVM_ERR_LIMIT_EXCEEDED:      return "QVM_ERR_LIMIT_EXCEEDED";
        case QVM_ERR_LOW_LEVEL_DISABLED:  return "QVM_ERR_LOW_LEVEL_DISABLED";
        case QVM_ERR_NULL_REFERENCE:      return "QVM_ERR_NULL_REFERENCE";
        case QVM_ERR_INVALID_CHAR:        return "QVM_ERR_INVALID_CHAR";
        case QVM_ERR_INVALID_BOOL:        return "QVM_ERR_INVALID_BOOL";
        case QVM_ERR_MODULE_INCOMPATIBLE: return "QVM_ERR_MODULE_INCOMPATIBLE";
        case QVM_ERR_IO:                  return "QVM_ERR_IO";
        case QVM_ERR_INTERNAL:            return "QVM_ERR_INTERNAL";
        default:                          return "QVM_ERR_UNKNOWN";
    }
}

const char *qvm_error_message(qvm_error err) {
    switch (err) {
        case QVM_OK:                      return "success";
        case QVM_ERR_INVALID_ARGUMENT:    return "invalid argument";
        case QVM_ERR_OUT_OF_MEMORY:       return "out of memory";
        case QVM_ERR_INVALID_BYTECODE:    return "invalid bytecode";
        case QVM_ERR_VERIFICATION_FAILED: return "verification failed";
        case QVM_ERR_TYPE_MISMATCH:       return "type mismatch";
        case QVM_ERR_STACK_OVERFLOW:      return "stack overflow";
        case QVM_ERR_CALL_DEPTH_EXCEEDED: return "call depth exceeded";
        case QVM_ERR_LIMIT_EXCEEDED:      return "limit exceeded";
        case QVM_ERR_LOW_LEVEL_DISABLED:  return "low-level mode disabled";
        case QVM_ERR_NULL_REFERENCE:      return "null reference";
        case QVM_ERR_INVALID_CHAR:        return "invalid char value";
        case QVM_ERR_INVALID_BOOL:        return "invalid bool value";
        case QVM_ERR_MODULE_INCOMPATIBLE: return "incompatible module representation";
        case QVM_ERR_IO:                  return "I/O error";
        case QVM_ERR_INTERNAL:            return "internal error";
        default:                          return "unknown error";
    }
}