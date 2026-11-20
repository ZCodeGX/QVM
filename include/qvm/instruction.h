#ifndef QVM_INSTRUCTION_H
#define QVM_INSTRUCTION_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/frames.h"
#include "qvm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QVM_INSN_NO_REG ((uint32_t)UINT32_MAX)

typedef enum qvm_opcode {
    QVM_OP_MOV = 1,
    QVM_OP_LOAD_CONST,
    QVM_OP_ARITH_ADD, QVM_OP_ARITH_SUB, QVM_OP_ARITH_MUL,
    QVM_OP_ARITH_DIV, QVM_OP_ARITH_MOD, QVM_OP_ARITH_NEG,
    QVM_OP_CMP_EQ, QVM_OP_CMP_NE, QVM_OP_CMP_LT,
    QVM_OP_CMP_LE, QVM_OP_CMP_GT, QVM_OP_CMP_GE,
    QVM_OP_LOGIC_AND, QVM_OP_LOGIC_OR, QVM_OP_LOGIC_NOT,
    QVM_OP_JMP, QVM_OP_JMP_IF, QVM_OP_JMP_IF_NOT,
    QVM_OP_CALL,
    QVM_OP_RETURN, QVM_OP_RETURN_VOID,
    QVM_OP_CAST,
    QVM_OP_STR_CONCAT, QVM_OP_STR_LEN,
    QVM_OP_STR_CHAR_AT, QVM_OP_STR_SLICE, QVM_OP_STR_CMP,
    QVM_OP_CALL_NATIVE
} qvm_opcode;

typedef struct qvm_instruction {
    qvm_opcode      opcode;
    qvm_type_id_t   type_id;
    uint32_t        dst;
    uint32_t        src1;
    uint32_t        src2;
    uint32_t        imm32;
    uint32_t       *args;
    uint16_t        arg_count;
} qvm_instruction;

const char   *qvm_opcode_name(qvm_opcode op);
qvm_type_id_t qvm_instruction_result_type(const qvm_instruction *ins);
qvm_error     qvm_instruction_validate(const qvm_instruction *ins, const qvm_frame *frame);

qvm_error     qvm_instruction_decode(const uint8_t *bytes, uint32_t len, qvm_instruction *out);
void          qvm_instruction_free(qvm_instruction *ins);

qvm_error     qvm_instruction_execute(const qvm_instruction *ins, qvm_frame *frame);

#ifdef __cplusplus
}
#endif

#endif

// ==========================================
// QMS-Zero Opcodes (Zero-Overhead Memory)
// ==========================================
#define QVM_OP_ALLOC      0x30  // Asignar memoria: ALLOC dst_reg, size, type_id
#define QVM_OP_FREE       0x31  // Liberar memoria: FREE src_reg
#define QVM_OP_LOAD_PTR   0x32  // Leer de puntero: LOAD_PTR dst_reg, src_ptr_reg, offset
#define QVM_OP_STORE_PTR  0x33  // Escribir en puntero: STORE_PTR dst_ptr_reg, src_reg, offset

