#ifndef QVM_ERROR_H
#define QVM_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum qvm_error {
    QVM_OK = 0,
    QVM_ERR_INVALID_ARGUMENT,
    QVM_ERR_OUT_OF_MEMORY,
    QVM_ERR_INVALID_BYTECODE,
    QVM_ERR_VERIFICATION_FAILED,
    QVM_ERR_TYPE_MISMATCH,
    QVM_ERR_STACK_OVERFLOW,
    QVM_ERR_CALL_DEPTH_EXCEEDED,
    QVM_ERR_LIMIT_EXCEEDED,
    QVM_ERR_LOW_LEVEL_DISABLED,
    QVM_ERR_NULL_REFERENCE,
    QVM_ERR_INVALID_CHAR,
    QVM_ERR_INVALID_BOOL,
    QVM_ERR_MODULE_INCOMPATIBLE,
    QVM_ERR_IO,
    QVM_ERR_INTERNAL
} qvm_error;

const char *qvm_error_name(qvm_error err);
const char *qvm_error_message(qvm_error err);

#ifdef __cplusplus
}
#endif

#endif /* QVM_ERROR_H */