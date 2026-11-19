/* QVM core - representacion de valores. Sin allocation, sin estado. */
#include <string.h>

#include "qvm/value.h"

qvm_value qvm_value_make(qvm_type_id_t type_id, uint64_t payload) {
    qvm_value v;
    v.payload   = payload;
    v.type_id   = type_id;
    v.flags     = QVM_VALUE_FLAG_NONE;
    v._reserved = 0;
    return v;
}

qvm_value qvm_value_null_ref(void) { return qvm_value_make(QVM_TYPE_REF, 0); }
qvm_value qvm_value_null_ptr(void) { return qvm_value_make(QVM_TYPE_PTR, 0); }

int8_t   qvm_value_as_i8  (const qvm_value *v) { return (int8_t)  v->payload; }
int16_t  qvm_value_as_i16 (const qvm_value *v) { return (int16_t) v->payload; }
int32_t  qvm_value_as_i32 (const qvm_value *v) { return (int32_t) v->payload; }
int64_t  qvm_value_as_i64 (const qvm_value *v) { return (int64_t) v->payload; }
uint8_t  qvm_value_as_u8  (const qvm_value *v) { return (uint8_t) v->payload; }
uint16_t qvm_value_as_u16 (const qvm_value *v) { return (uint16_t)v->payload; }
uint32_t qvm_value_as_u32 (const qvm_value *v) { return (uint32_t)v->payload; }
uint64_t qvm_value_as_u64 (const qvm_value *v) { return           v->payload; }

/* f32/f64: reinterpretacion bit a bit del payload (QVM TS 4.4). */
float qvm_value_as_f32(const qvm_value *v) {
    uint32_t bits = (uint32_t)v->payload;
    float    out;
    memcpy(&out, &bits, sizeof out);
    return out;
}

double qvm_value_as_f64(const qvm_value *v) {
    uint64_t bits = v->payload;
    double   out;
    memcpy(&out, &bits, sizeof out);
    return out;
}

int      qvm_value_as_bool(const qvm_value *v) { return v->payload != 0; }
uint32_t qvm_value_as_char(const qvm_value *v) { return (uint32_t)v->payload; }

int qvm_value_is_null_ref(const qvm_value *v) {
    return v->type_id == QVM_TYPE_REF && v->payload == 0;
}

int qvm_value_is_null_ptr(const qvm_value *v) {
    return v->type_id == QVM_TYPE_PTR && v->payload == 0;
}