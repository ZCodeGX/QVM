#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/type_table.h"

static void test_add_and_find(void) {
    qvm_type_table table = {0};

    qvm_type_field fields[2] = {
        { 0, QVM_TYPE_I32 },
        { 4, QVM_TYPE_I32 }
    };

    qvm_type_entry entry = {0};
    entry.id          = 0x1000;
    entry.name        = "Point";
    entry.category    = QVM_TYPE_CAT_STRUCT;
    entry.size        = 8;
    entry.align       = 4;
    entry.flags       = QVM_TYPE_FLAG_BY_VALUE;
    entry.fields      = fields;
    entry.field_count = 2;

    assert(qvm_type_table_add(&table, &entry) == QVM_OK);
    assert(table.count == 1);

    const qvm_type_entry *found = qvm_type_table_find(&table, 0x1000);
    assert(found != NULL);
    assert(strcmp(found->name, "Point") == 0);
    assert(found->category == QVM_TYPE_CAT_STRUCT);
    assert(found->field_count == 2);
    assert(found->fields[0].offset == 0);
    assert(found->fields[1].offset == 4);

    const qvm_type_entry *by_name = qvm_type_table_find_by_name(&table, "Point");
    assert(by_name == found);

    assert(qvm_type_table_find(&table, 0x9999) == NULL);
    assert(qvm_type_table_find_by_name(&table, "Unknown") == NULL);

    qvm_type_table_clear(&table);
    assert(table.count == 0);
    assert(table.entries == NULL);
}

static void test_duplicate_rejected(void) {
    qvm_type_table table = {0};
    qvm_type_entry e1 = {0};
    e1.id       = 0x1000;
    e1.name     = "A";
    e1.category = QVM_TYPE_CAT_OPAQUE;

    assert(qvm_type_table_add(&table, &e1) == QVM_OK);
    assert(qvm_type_table_add(&table, &e1) == QVM_ERR_INVALID_ARGUMENT);
    assert(table.count == 1);

    qvm_type_table_clear(&table);
}

static void test_opaque_type(void) {
    qvm_type_table table = {0};
    qvm_type_entry entry = {0};
    entry.id       = 0x1001;
    entry.name     = "FileHandle";
    entry.category = QVM_TYPE_CAT_OPAQUE;
    entry.size     = 0;
    entry.align    = 0;
    entry.flags    = QVM_TYPE_FLAG_BY_REFERENCE;

    assert(qvm_type_table_add(&table, &entry) == QVM_OK);
    const qvm_type_entry *found = qvm_type_table_find(&table, 0x1001);
    assert(found != NULL);
    assert(found->category == QVM_TYPE_CAT_OPAQUE);
    assert(found->flags == QVM_TYPE_FLAG_BY_REFERENCE);

    qvm_type_table_clear(&table);
}

static void test_struct_with_multiple_fields(void) {
    qvm_type_table table = {0};
    qvm_type_field fields[3] = {
        { 0,  QVM_TYPE_I32 },
        { 4,  QVM_TYPE_F64 },
        { 12, QVM_TYPE_BOOL }
    };

    qvm_type_entry entry = {0};
    entry.id          = 0x1002;
    entry.name        = "Entity";
    entry.category    = QVM_TYPE_CAT_STRUCT;
    entry.size        = 16;
    entry.align       = 8;
    entry.flags       = QVM_TYPE_FLAG_BY_VALUE | QVM_TYPE_FLAG_COPY_SEMANTICS;
    entry.fields      = fields;
    entry.field_count = 3;

    assert(qvm_type_table_add(&table, &entry) == QVM_OK);
    const qvm_type_entry *found = qvm_type_table_find(&table, 0x1002);
    assert(found != NULL);
    assert(found->field_count == 3);
    assert(found->fields[0].type_id == QVM_TYPE_I32);
    assert(found->fields[1].type_id == QVM_TYPE_F64);
    assert(found->fields[2].type_id == QVM_TYPE_BOOL);
    assert(found->size == 16);
    assert(found->align == 8);

    qvm_type_table_clear(&table);
}

static void test_deep_copy(void) {
    qvm_type_table table = {0};
    qvm_type_field fields[1] = { { 0, QVM_TYPE_I32 } };
    qvm_type_entry entry = {0};
    entry.id          = 0x1003;
    entry.name        = "Original";
    entry.category    = QVM_TYPE_CAT_STRUCT;
    entry.fields      = fields;
    entry.field_count = 1;

    assert(qvm_type_table_add(&table, &entry) == QVM_OK);
    const qvm_type_entry *found = qvm_type_table_find(&table, 0x1003);
    assert(found != NULL);
    assert(found->fields != fields);
    assert(found->name != entry.name);
    assert(found->fields[0].offset == 0);
    assert(found->fields[0].type_id == QVM_TYPE_I32);

    qvm_type_table_clear(&table);
}

static void test_null_args(void) {
    assert(qvm_type_table_find(NULL, 0) == NULL);
    assert(qvm_type_table_find_by_name(NULL, "x") == NULL);
    assert(qvm_type_table_add(NULL, NULL) == QVM_ERR_INVALID_ARGUMENT);
    qvm_type_table_clear(NULL);
}

int main(void) {
    test_add_and_find();
    test_duplicate_rejected();
    test_opaque_type();
    test_struct_with_multiple_fields();
    test_deep_copy();
    test_null_args();
    printf("test_type_table: OK\n");
    return 0;
}
