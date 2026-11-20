#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/instruction.h"

static void test_header_too_short(void) {
    qvm_instruction ins = {0};
    uint8_t buf[3] = {1, 2, 3};
    assert(qvm_instruction_decode(buf, 3, &ins) == QVM_ERR_INVALID_BYTECODE);
    assert(qvm_instruction_decode(NULL, 4, &ins) == QVM_ERR_INVALID_BYTECODE);
    assert(qvm_instruction_decode(buf, 4, NULL) == QVM_ERR_INVALID_BYTECODE);
}

static void test_invalid_opcode(void) {
    qvm_instruction ins = {0};
    uint8_t buf[4] = {0, 0, 0, 0};
    assert(qvm_instruction_decode(buf, 4, &ins) == QVM_ERR_INVALID_BYTECODE);
    uint8_t buf2[4] = {200, 0, 0, 0};
    assert(qvm_instruction_decode(buf2, 4, &ins) == QVM_ERR_INVALID_BYTECODE);
}

static void test_return_void(void) {
    qvm_instruction ins = {0};
    uint8_t buf[4] = {QVM_OP_RETURN_VOID, 0xFF, 0x00, 0x00};
    assert(qvm_instruction_decode(buf, 4, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_RETURN_VOID);
    assert(ins.dst == QVM_INSN_NO_REG);
    assert(ins.type_id == 0);
}

static void test_return_src(void) {
    qvm_instruction ins = {0};
    uint8_t buf[5] = {QVM_OP_RETURN, 0xFF, 0x04, 0x00, 7};
    assert(qvm_instruction_decode(buf, 5, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_RETURN);
    assert(ins.src1 == 7);
}

static void test_mov(void) {
    qvm_instruction ins = {0};
    uint8_t buf[5] = {QVM_OP_MOV, 0, 0x04, 0x00, 1};
    assert(qvm_instruction_decode(buf, 5, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_MOV);
    assert(ins.dst == 0);
    assert(ins.src1 == 1);
    assert(ins.type_id == QVM_TYPE_I32);
}

static void test_arith_add(void) {
    qvm_instruction ins = {0};
    uint8_t buf[6] = {QVM_OP_ARITH_ADD, 0, 0x04, 0x00, 1, 2};
    assert(qvm_instruction_decode(buf, 6, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_ARITH_ADD);
    assert(ins.dst == 0 && ins.src1 == 1 && ins.src2 == 2);
}

static void test_load_const(void) {
    qvm_instruction ins = {0};
    uint8_t buf[8] = {QVM_OP_LOAD_CONST, 0, 0x04, 0x00, 42, 0, 0, 0};
    assert(qvm_instruction_decode(buf, 8, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_LOAD_CONST);
    assert(ins.imm32 == 42);
}

static void test_jmp(void) {
    qvm_instruction ins = {0};
    uint8_t buf[8] = {QVM_OP_JMP, 0xFF, 0x00, 0x00, 100, 0, 0, 0};
    assert(qvm_instruction_decode(buf, 8, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_JMP);
    assert(ins.dst == QVM_INSN_NO_REG);
    assert(ins.imm32 == 100);
}

static void test_jmp_if(void) {
    qvm_instruction ins = {0};
    uint8_t buf[9] = {QVM_OP_JMP_IF, 0xFF, 0x01, 0x00, 3, 100, 0, 0, 0};
    assert(qvm_instruction_decode(buf, 9, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_JMP_IF);
    assert(ins.src1 == 3);
    assert(ins.imm32 == 100);
}

static void test_call(void) {
    qvm_instruction ins = {0};
    uint8_t buf[11] = {QVM_OP_CALL, 0, 0x04, 0x00, 5, 0, 0, 0, 2, 1, 2};
    assert(qvm_instruction_decode(buf, 11, &ins) == QVM_OK);
    assert(ins.opcode == QVM_OP_CALL);
    assert(ins.imm32 == 5);
    assert(ins.arg_count == 2);
    assert(ins.args[0] == 1 && ins.args[1] == 2);
    qvm_instruction_free(&ins);
    assert(ins.arg_count == 0);
}

static void test_truncated(void) {
    qvm_instruction ins = {0};
    uint8_t buf_add[5] = {QVM_OP_ARITH_ADD, 0, 0x04, 0x00, 1};
    assert(qvm_instruction_decode(buf_add, 5, &ins) == QVM_ERR_INVALID_BYTECODE);

    uint8_t buf_load[7] = {QVM_OP_LOAD_CONST, 0, 0x04, 0x00, 1, 0, 0};
    assert(qvm_instruction_decode(buf_load, 7, &ins) == QVM_ERR_INVALID_BYTECODE);

    uint8_t buf_call[10] = {QVM_OP_CALL, 0, 0x04, 0x00, 5, 0, 0, 0, 2, 1};
    assert(qvm_instruction_decode(buf_call, 10, &ins) == QVM_ERR_INVALID_BYTECODE);
}

int main(void) {
    test_header_too_short();
    test_invalid_opcode();
    test_return_void();
    test_return_src();
    test_mov();
    test_arith_add();
    test_load_const();
    test_jmp();
    test_jmp_if();
    test_call();
    test_truncated();

    printf("test_decode: OK\n");
    return 0;
}
