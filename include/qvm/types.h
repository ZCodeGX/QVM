#ifndef QVM_TYPES_H
#define QVM_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t qvm_type_id_t;

/* Tipos primitivos nativos (QVM TS 3.3). */
#define QVM_TYPE_VOID   ((qvm_type_id_t)0x0000u)
#define QVM_TYPE_BOOL   ((qvm_type_id_t)0x0001u)
#define QVM_TYPE_I8     ((qvm_type_id_t)0x0002u)
#define QVM_TYPE_I16    ((qvm_type_id_t)0x0003u)
#define QVM_TYPE_I32    ((qvm_type_id_t)0x0004u)
#define QVM_TYPE_I64    ((qvm_type_id_t)0x0005u)
#define QVM_TYPE_I128   ((qvm_type_id_t)0x0006u)
#define QVM_TYPE_U8     ((qvm_type_id_t)0x0007u)
#define QVM_TYPE_U16    ((qvm_type_id_t)0x0008u)
#define QVM_TYPE_U32    ((qvm_type_id_t)0x0009u)
#define QVM_TYPE_U64    ((qvm_type_id_t)0x000Au)
#define QVM_TYPE_U128   ((qvm_type_id_t)0x000Bu)
#define QVM_TYPE_F32    ((qvm_type_id_t)0x000Cu)
#define QVM_TYPE_F64    ((qvm_type_id_t)0x000Du)
#define QVM_TYPE_CHAR   ((qvm_type_id_t)0x000Eu)
#define QVM_TYPE_REF    ((qvm_type_id_t)0x000Fu)
#define QVM_TYPE_PTR    ((qvm_type_id_t)0x0010u)

/* Rangos reservados (QVM TS 3.2). */
#define QVM_TYPE_ID_MIN_NATIVE    ((qvm_type_id_t)0x0001u)
#define QVM_TYPE_ID_MAX_NATIVE    ((qvm_type_id_t)0x00FFu)
#define QVM_TYPE_ID_MIN_RESERVED  ((qvm_type_id_t)0x0100u)
#define QVM_TYPE_ID_MAX_RESERVED  ((qvm_type_id_t)0x0FFFu)
#define QVM_TYPE_ID_MIN_EXTENDED  ((qvm_type_id_t)0x1000u)
#define QVM_TYPE_ID_MAX_EXTENDED  ((qvm_type_id_t)0xFFFEu)
#define QVM_TYPE_ID_INVALID       ((qvm_type_id_t)0xFFFFu)

typedef enum qvm_type_category {
    QVM_TYPE_CAT_PRIMITIVE = 0,
    QVM_TYPE_CAT_STRUCT    = 1,
    QVM_TYPE_CAT_ARRAY     = 2,
    QVM_TYPE_CAT_ENUM      = 3,
    QVM_TYPE_CAT_ALIAS     = 4,
    QVM_TYPE_CAT_OPAQUE    = 5
} qvm_type_category;

typedef struct qvm_type_field {
    uint32_t      offset;
    qvm_type_id_t type_id;
} qvm_type_field;

/* Tabla de tipos extendidos del modulo .qcb (QVM TS 3.4). */
typedef struct qvm_type_entry {
    qvm_type_id_t          id;
    uint8_t                category;
    uint8_t                flags;
    uint16_t               field_count;
    uint32_t               size;
    uint32_t               align;
    const char            *name;
    const qvm_type_field  *fields;
} qvm_type_entry;

typedef struct qvm_type_table {
    const qvm_type_entry *entries;
    uint32_t              count;
} qvm_type_table;

int    qvm_type_is_native  (qvm_type_id_t id);
int    qvm_type_is_extended(qvm_type_id_t id);
int    qvm_type_is_valid   (qvm_type_id_t id);
size_t qvm_type_slot_count (qvm_type_id_t id);
size_t qvm_type_native_size(qvm_type_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* QVM_TYPES_H */