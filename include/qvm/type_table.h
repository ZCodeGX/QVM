#ifndef QVM_TYPE_TABLE_H
#define QVM_TYPE_TABLE_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QVM_TYPE_FLAG_BY_VALUE       0x01u
#define QVM_TYPE_FLAG_BY_REFERENCE   0x02u
#define QVM_TYPE_FLAG_COPY_SEMANTICS 0x04u
#define QVM_TYPE_FLAG_DROP_SEMANTICS 0x08u

const qvm_type_entry *qvm_type_table_find(const qvm_type_table *table, qvm_type_id_t id);
const qvm_type_entry *qvm_type_table_find_by_name(const qvm_type_table *table, const char *name);
qvm_error             qvm_type_table_add(qvm_type_table *table, const qvm_type_entry *entry);
void                  qvm_type_table_clear(qvm_type_table *table);

#ifdef __cplusplus
}
#endif

#endif
