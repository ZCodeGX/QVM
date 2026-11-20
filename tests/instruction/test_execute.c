#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

static qvm_frame *make_frame(qvm_type_id_t *types, uint32_t n) {
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, n, types, NULL, &err);
    assert(f != NULL);
    return f;
}

static void test_mov(void) {
    qvm_type_id_t t[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *f = make_frame(t, 2);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 42));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_MOV;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 1; ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v; qvm_register_read(f, 1, &v);
    assert(v.payload == 42);
    qvm_frame_destroy(f);
}

static void test_arith_i32(void) {
    qvm_type_id_t t[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *f = make_frame(t, 3);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 100));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_I32, 30));
    qvm_instruction ins = {0};
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 2; ins.src1 = 0; ins.src2 = 1;

    ins.opcode = QVM_OP_ARITH_ADD;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v; qvm_register_read(f, 2, &v);
    assert(v.payload == 130);

    ins.opcode = QVM_OP_ARITH_SUB;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 70);

    ins.opcode = QVM_OP_ARITH_MUL;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 3000);

    ins.opcode = QVM_OP_ARITH_DIV;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 3);

    ins.opcode = QVM_OP_ARITH_MOD;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 10);

    qvm_frame_destroy(f);
}

static void test_arith_div_by_zero(void) {
    qvm_type_id_t t[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *f = make_frame(t, 3);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_I32, 0));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_ARITH_DIV;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 2; ins.src1 = 0; ins.src2 = 1;
    assert(qvm_instruction_execute(&ins, f) == QVM_ERR_INVALID_ARGUMENT);
    qvm_frame_destroy(f);
}

static void test_arith_f64(void) {
    qvm_type_id_t t[3] = { QVM_TYPE_F64, QVM_TYPE_F64, QVM_TYPE_F64 };
    qvm_frame *f = make_frame(t, 3);
    double va = 2.5, vb = 1.5;
    qvm_value a = qvm_value_make(QVM_TYPE_F64, 0);
    qvm_value b = qvm_value_make(QVM_TYPE_F64, 0);
    memcpy(&a.payload, &va, sizeof(double));
    memcpy(&b.payload, &vb, sizeof(double));
    qvm_register_write(f, 0, a);
    qvm_register_write(f, 1, b);

    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_ARITH_ADD;
    ins.type_id = QVM_TYPE_F64;
    ins.dst = 2; ins.src1 = 0; ins.src2 = 1;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);

    qvm_value v; qvm_register_read(f, 2, &v);
    double r; memcpy(&r, &v.payload, sizeof(double));
    assert(r == 4.0);
    qvm_frame_destroy(f);
}

static void test_arith_i128(void) {
    qvm_type_id_t t[6] = { QVM_TYPE_I128, QVM_TYPE_I128, QVM_TYPE_I128,
                           QVM_TYPE_I128, QVM_TYPE_I128, QVM_TYPE_I128 };
    qvm_frame *f = make_frame(t, 6);
    qvm_register_write_pair(f, 0, 0xFFFFFFFFFFFFFFFFULL, 0);
    qvm_register_write_pair(f, 2, 1, 0);

    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_ARITH_ADD;
    ins.type_id = QVM_TYPE_I128;
    ins.dst = 4; ins.src1 = 0; ins.src2 = 2;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);

    uint64_t lo, hi;
    qvm_register_read_pair(f, 4, &lo, &hi);
    assert(lo == 0 && hi == 1);
    qvm_frame_destroy(f);
}

static void test_cmp(void) {
    qvm_type_id_t t[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_frame *f = make_frame(t, 3);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_I32, 20));

    qvm_instruction ins = {0};
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 2; ins.src1 = 0; ins.src2 = 1;

    ins.opcode = QVM_OP_CMP_LT;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v; qvm_register_read(f, 2, &v);
    assert(v.payload == 1);

    ins.opcode = QVM_OP_CMP_GT;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 0);

    ins.opcode = QVM_OP_CMP_EQ;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 0);
    qvm_frame_destroy(f);
}

static void test_logic(void) {
    qvm_type_id_t t[3] = { QVM_TYPE_BOOL, QVM_TYPE_BOOL, QVM_TYPE_BOOL };
    qvm_frame *f = make_frame(t, 3);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_BOOL, 1));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_BOOL, 0));

    qvm_instruction ins = {0};
    ins.type_id = QVM_TYPE_BOOL;
    ins.dst = 2; ins.src1 = 0; ins.src2 = 1;

    ins.opcode = QVM_OP_LOGIC_AND;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v; qvm_register_read(f, 2, &v);
    assert(v.payload == 0);

    ins.opcode = QVM_OP_LOGIC_OR;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 1);

    ins.opcode = QVM_OP_LOGIC_NOT;
    ins.src2 = QVM_INSN_NO_REG;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_register_read(f, 2, &v);
    assert(v.payload == 0);
    qvm_frame_destroy(f);
}

static void test_neg(void) {
    qvm_type_id_t t[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_frame *f = make_frame(t, 2);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 42));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_ARITH_NEG;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 1; ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v; qvm_register_read(f, 1, &v);
    assert((int32_t)v.payload == -42);
    qvm_frame_destroy(f);
}

static void test_cast_i32_to_i64(void) {
    qvm_type_id_t t[2] = { QVM_TYPE_I32, QVM_TYPE_I64 };
    qvm_frame *f = make_frame(t, 2);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 12345));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_CAST;
    ins.type_id = QVM_TYPE_I64;
    ins.dst = 1; ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v; qvm_register_read(f, 1, &v);
    assert(v.type_id == QVM_TYPE_I64);
    assert(v.payload == 12345);
    qvm_frame_destroy(f);
}

static void test_cast_f64_to_i32(void) {
    qvm_type_id_t t[2] = { QVM_TYPE_F64, QVM_TYPE_I32 };
    qvm_frame *f = make_frame(t, 2);
    double v = 3.9;
    qvm_value fv = qvm_value_make(QVM_TYPE_F64, 0);
    memcpy(&fv.payload, &v, sizeof(double));
    qvm_register_write(f, 0, fv);
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_CAST;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 1; ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value r; qvm_register_read(f, 1, &r);
    assert(r.type_id == QVM_TYPE_I32);
    assert(r.payload == 3);
    qvm_frame_destroy(f);
}

static void test_cast_invalid_char(void) {
    qvm_type_id_t t[2] = { QVM_TYPE_I64, QVM_TYPE_CHAR };
    qvm_frame *f = make_frame(t, 2);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I64, 0x110000));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_CAST;
    ins.type_id = QVM_TYPE_CHAR;
    ins.dst = 1; ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_ERR_INVALID_CHAR);
    qvm_frame_destroy(f);
}

static void test_null_args(void) {
    qvm_instruction ins = {0};
    assert(qvm_instruction_execute(NULL, NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_instruction_execute(&ins, NULL) == QVM_ERR_INVALID_ARGUMENT);
}

int main(void) {
    test_mov();
    test_arith_i32();
    test_arith_div_by_zero();
    test_arith_f64();
    test_arith_i128();
    test_cmp();
    test_logic();
    test_neg();
    test_cast_i32_to_i64();
    test_cast_f64_to_i32();
    test_cast_invalid_char();
    test_null_args();
    printf("test_execute: OK\n");
    return 0;
}
