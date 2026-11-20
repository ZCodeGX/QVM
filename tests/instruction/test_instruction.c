#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

static void test_opcode_name(void) {
    assert(qvm_opcode_name(QVM_OP_MOV) != NULL);
    assert(strcmp(qvm_opcode_name(QVM_OP_MOV), "MOV") == 0);
    assert(strcmp(qvm_opcode_name(QVM_OP_CALL), "CALL") == 0);
    assert(strcmp(qvm_opcode_name(QVM_OP_RETURN_VOID), "RETURN_VOID") == 0);
}

static void test_result_type(void) {
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_CMP_EQ;
    assert(qvm_instruction_result_type(&ins) == QVM_TYPE_BOOL);
    ins.opcode = QVM_OP_LOGIC_NOT;
    assert(qvm_instruction_result_type(&ins) == QVM_TYPE_BOOL);
    ins.opcode = QVM_OP_JMP;
    assert(qvm_instruction_result_type(&ins) == QVM_TYPE_VOID);
    ins.opcode = QVM_OP_RETURN_VOID;
    assert(qvm_instruction_result_type(&ins) == QVM_TYPE_VOID);
    ins.opcode = QVM_OP_CAST;
    ins.type_id = QVM_TYPE_I64;
    assert(qvm_instruction_result_type(&ins) == QVM_TYPE_I64);
    assert(qvm_instruction_result_type(NULL) == QVM_TYPE_VOID);
}

static void test_validate(void) {
    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 3, types, NULL, &err);
    assert(f != NULL);
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_MOV;
    ins.dst = 0;
    ins.src1 = 1;
    ins.src2 = QVM_INSN_NO_REG;
    assert(qvm_instruction_validate(&ins, f) == QVM_OK);
    ins.dst = 99;
    assert(qvm_instruction_validate(&ins, f) == QVM_ERR_INVALID_ARGUMENT);
    ins.dst = 0;
    ins.src1 = 99;
    assert(qvm_instruction_validate(&ins, f) == QVM_ERR_INVALID_ARGUMENT);
    qvm_frame_destroy(f);
    assert(qvm_instruction_validate(NULL, f) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_instruction_validate(&ins, NULL) == QVM_ERR_INVALID_ARGUMENT);
}

static void test_validate_args(void) {
    qvm_type_id_t types[4] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 4, types, NULL, &err);
    assert(f != NULL);
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_CALL;
    ins.dst = 0;
    ins.src1 = QVM_INSN_NO_REG;
    ins.src2 = QVM_INSN_NO_REG;
    ins.args[0] = 1;
    ins.args[1] = 2;
    ins.arg_count = 2;
    assert(qvm_instruction_validate(&ins, f) == QVM_OK);
    ins.args[0] = 99;
    assert(qvm_instruction_validate(&ins, f) == QVM_ERR_INVALID_ARGUMENT);
    qvm_frame_destroy(f);
}

static void test_execute_mov(void) {
    qvm_type_id_t types[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 2, types, NULL, &err);
    assert(f != NULL);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 42));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_MOV;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 1;
    ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v;
    qvm_register_read(f, 1, &v);
    assert(v.payload == 42);
    qvm_frame_destroy(f);
}

static void test_execute_arith(void) {
    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 3, types, NULL, &err);
    assert(f != NULL);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_I32, 20));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_ARITH_ADD;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 2;
    ins.src1 = 0;
    ins.src2 = 1;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v;
    qvm_register_read(f, 2, &v);
    assert(v.payload == 30);
    qvm_frame_destroy(f);
}

static void test_execute_cmp(void) {
    qvm_type_id_t types[3] = { QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 3, types, NULL, &err);
    assert(f != NULL);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 10));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_I32, 20));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_CMP_LT;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 2;
    ins.src1 = 0;
    ins.src2 = 1;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v;
    qvm_register_read(f, 2, &v);
    assert(v.payload == 1);
    qvm_frame_destroy(f);
}

static void test_execute_logic(void) {
    qvm_type_id_t types[3] = { QVM_TYPE_BOOL, QVM_TYPE_BOOL, QVM_TYPE_BOOL };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 3, types, NULL, &err);
    assert(f != NULL);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_BOOL, 1));
    qvm_register_write(f, 1, qvm_value_make(QVM_TYPE_BOOL, 0));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_LOGIC_AND;
    ins.type_id = QVM_TYPE_BOOL;
    ins.dst = 2;
    ins.src1 = 0;
    ins.src2 = 1;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v;
    qvm_register_read(f, 2, &v);
    assert(v.payload == 0);
    qvm_frame_destroy(f);
}

static void test_execute_neg(void) {
    qvm_type_id_t types[2] = { QVM_TYPE_I32, QVM_TYPE_I32 };
    qvm_error err;
    qvm_frame *f = qvm_frame_create(0, 2, types, NULL, &err);
    assert(f != NULL);
    qvm_register_write(f, 0, qvm_value_make(QVM_TYPE_I32, 42));
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_ARITH_NEG;
    ins.type_id = QVM_TYPE_I32;
    ins.dst = 1;
    ins.src1 = 0;
    assert(qvm_instruction_execute(&ins, f) == QVM_OK);
    qvm_value v;
    qvm_register_read(f, 1, &v);
    assert((int32_t)v.payload == -42);
    qvm_frame_destroy(f);
}

static void test_execute_null_args(void) {
    qvm_instruction ins = {0};
    ins.opcode = QVM_OP_MOV;
    ins.dst = 0;
    ins.src1 = 1;
    assert(qvm_instruction_execute(&ins, NULL) == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_instruction_execute(NULL, NULL) == QVM_ERR_INVALID_ARGUMENT);
}

int main(void) {
    test_opcode_name();
    test_result_type();
    test_validate();
    test_validate_args();
    test_execute_mov();
    test_execute_arith();
    test_execute_cmp();
    test_execute_logic();
    test_execute_neg();
    test_execute_null_args();
    printf("test_instruction: OK\n");
    return 0;
}
