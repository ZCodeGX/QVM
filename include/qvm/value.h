#ifndef QVM_VALUE_H
#define QVM_VALUE_H

#include <stddef.h>
#include <stdint.h>

#include "qvm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Slot de 8 bytes: unidad minima de almacenamiento (QVM TS 4.1). */
typedef struct qvm_slot {
    uint64_t payload;
} qvm_slot;

/* Valor con tag opcional para modo seguro (QVM TS 4.2). */
typedef struct qvm_value {
    uint64_t      payload;
    qvm_type_id_t type_id;
    uint8_t       flags;
    uint8_t       _reserved;
} qvm_value;

#define QVM_VALUE_FLAG_NONE      ((uint8_t)0x00u)
#define QVM_VALUE_FLAG_CONST     ((uint8_t)0x01u)
#define QVM_VALUE_FLAG_OWNED     ((uint8_t)0x02u)
#define QVM_VALUE_FLAG_BORROWED  ((uint8_t)0x04u)
#define QVM_VALUE_FLAG_GCQ_ROOT  ((uint8_t)0x08u)

qvm_value qvm_value_make    (qvm_type_id_t type_id, uint64_t payload);
qvm_value qvm_value_null_ref(void);
qvm_value qvm_value_null_ptr(void);

int8_t   qvm_value_as_i8  (const qvm_value *v);
int16_t  qvm_value_as_i16 (const qvm_value *v);
int32_t  qvm_value_as_i32 (const qvm_value *v);
int64_t  qvm_value_as_i64 (const qvm_value *v);
uint8_t  qvm_value_as_u8  (const qvm_value *v);
uint16_t qvm_value_as_u16 (const qvm_value *v);
uint32_t qvm_value_as_u32 (const qvm_value *v);
uint64_t qvm_value_as_u64 (const qvm_value *v);
float    qvm_value_as_f32 (const qvm_value *v);
double   qvm_value_as_f64 (const qvm_value *v);
int      qvm_value_as_bool(const qvm_value *v);
uint32_t qvm_value_as_char(const qvm_value *v);

int qvm_value_is_null_ref(const qvm_value *v);
int qvm_value_is_null_ptr(const qvm_value *v);


/* --- Blindaje de ABI (Fase 1, A1.6). No mover ni reordenar campos. --- */
#ifndef __cplusplus
#define QVM_ABI_VALUE_SIZE 16
_Static_assert(sizeof(qvm_value) == 16, "ABI: qvm_value debe medir 16 bytes");
_Static_assert(offsetof(qvm_value, payload) == 0, "ABI: payload debe estar en offset 0");
_Static_assert(offsetof(qvm_value, type_id) == 8, "ABI: type_id debe estar en offset 8");
_Static_assert(offsetof(qvm_value, flags) == 10, "ABI: flags debe estar en offset 10");
#endif

#ifdef __cplusplus
}
#endif

#endif /* QVM_VALUE_H */