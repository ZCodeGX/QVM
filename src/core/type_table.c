#include <stdlib.h>
#include <string.h>
#include "qvm/type_table.h"

const qvm_type_entry *qvm_type_table_find(const qvm_type_table *table, qvm_type_id_t id) {
    if (!table) return NULL;
    for (uint32_t i = 0; i < table->count; ++i) {
        if (table->entries[i].id == id) return &table->entries[i];
    }
    return NULL;
}

const qvm_type_entry *qvm_type_table_find_by_name(const qvm_type_table *table, const char *name) {
    if (!table || !name) return NULL;
    for (uint32_t i = 0; i < table->count; ++i) {
        if (table->entries[i].name && strcmp(table->entries[i].name, name) == 0) {
            return &table->entries[i];
        }
    }
    return NULL;
}

qvm_error qvm_type_table_add(qvm_type_table *table, const qvm_type_entry *entry) {
    if (!table || !entry) return QVM_ERR_INVALID_ARGUMENT;
    if (qvm_type_table_find(table, entry->id) != NULL) {
        return QVM_ERR_INVALID_ARGUMENT;
    }

    qvm_type_entry *mutable_entries = (qvm_type_entry *)table->entries;
    qvm_type_entry *new_entries = (qvm_type_entry *)realloc(
        mutable_entries, (table->count + 1) * sizeof(qvm_type_entry));
    if (!new_entries) return QVM_ERR_OUT_OF_MEMORY;

    new_entries[table->count] = *entry;
    
    if (entry->field_count > 0 && entry->fields) {
        qvm_type_field *fields_copy = (qvm_type_field *)malloc(
            entry->field_count * sizeof(qvm_type_field));
        if (!fields_copy) return QVM_ERR_OUT_OF_MEMORY;
        memcpy(fields_copy, entry->fields, entry->field_count * sizeof(qvm_type_field));
        new_entries[table->count].fields = fields_copy;
    } else {
        new_entries[table->count].fields = NULL;
    }

    if (entry->name) {
        size_t name_len = strlen(entry->name) + 1;
        char *name_copy = (char *)malloc(name_len);
        if (!name_copy) {
            free((void *)new_entries[table->count].fields);
            return QVM_ERR_OUT_OF_MEMORY;
        }
        memcpy(name_copy, entry->name, name_len);
        new_entries[table->count].name = name_copy;
    }

    table->entries = new_entries;
    table->count++;
    return QVM_OK;
}

void qvm_type_table_clear(qvm_type_table *table) {
    if (!table) return;
    if (table->entries) {
        qvm_type_entry *mutable_entries = (qvm_type_entry *)table->entries;
        for (uint32_t i = 0; i < table->count; ++i) {
            free((void *)mutable_entries[i].name);
            free((void *)mutable_entries[i].fields);
        }
        free(mutable_entries);
    }
    table->entries = NULL;
    table->count   = 0;
}
