#ifndef QVM_STRING_H
#define QVM_STRING_H

#include <stddef.h>
#include <stdint.h>
#include "qvm/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_string {
    const char *data;
    uint32_t    len;
    uint32_t    ref_count;
} qvm_string;

qvm_string *qvm_string_create(const char *data, uint32_t len, qvm_error *err);
qvm_string *qvm_string_create_from_cstr(const char *cstr, qvm_error *err);
qvm_string *qvm_string_retain(qvm_string *s);
void        qvm_string_release(qvm_string *s);

qvm_string *qvm_string_concat(const qvm_string *a, const qvm_string *b, qvm_error *err);
qvm_string *qvm_string_slice(const qvm_string *s, uint32_t start, uint32_t end, qvm_error *err);
uint32_t    qvm_string_len(const qvm_string *s);
int32_t     qvm_string_char_at(const qvm_string *s, uint32_t index, qvm_error *err);
int         qvm_string_compare(const qvm_string *a, const qvm_string *b);
int         qvm_string_equals(const qvm_string *a, const qvm_string *b);

#ifdef __cplusplus
}
#endif

#endif
