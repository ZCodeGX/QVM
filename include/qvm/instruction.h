#ifndef QVM_INSTRUCTION_H
#define QVM_INSTRUCTION_H

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QVM_INSN_NO_REG 0xFF

typedef enum qvm_opcode {
    // Control de flujo
    QVM_OP_JMP          = 0x01,
    QVM_OP_JMP_IF       = 0x02,
    QVM_OP_JMP_IF_NOT   = 0x03,
    QVM_OP_CALL         = 0x04,
    QVM_OP_CALL_NATIVE  = 0x05,
    QVM_OP_RETURN       = 0x06,
    QVM_OP_RETURN_VOID  = 0x07,
    
    // Movimiento y constantes
    QVM_OP_MOV          = 0x10,
    QVM_OP_LOAD_CONST   = 0x11,
    
    // Aritmética
    QVM_OP_ARITH_ADD    = 0x20,
    QVM_OP_ARITH_SUB    = 0x21,
    QVM_OP_ARITH_MUL    = 0x22,
    QVM_OP_ARITH_DIV    = 0x23,
    QVM_OP_ARITH_MOD    = 0x24,
    QVM_OP_ARITH_NEG    = 0x25,
    
    // Comparación
    QVM_OP_CMP_EQ       = 0x30,
    QVM_OP_CMP_NE       = 0x31,
    QVM_OP_CMP_LT       = 0x32,
    QVM_OP_CMP_LE       = 0x33,
    QVM_OP_CMP_GT       = 0x34,
    QVM_OP_CMP_GE       = 0x35,
    
    // Lógica
    QVM_OP_LOGIC_AND    = 0x40,
    QVM_OP_LOGIC_OR     = 0x41,
    QVM_OP_LOGIC_NOT    = 0x42,
    
    // Conversión
    QVM_OP_CAST         = 0x50,
    
    // Strings
    QVM_OP_STR_CONCAT   = 0x60,
    QVM_OP_STR_LEN      = 0x61,
    QVM_OP_STR_CHAR_AT  = 0x62,
    QVM_OP_STR_SLICE    = 0x63,
    QVM_OP_STR_CMP      = 0x64,
    
    // ==========================================
    // QMS-Zero Opcodes (Zero-Overhead Memory)
    // ==========================================
    QVM_OP_ALLOC        = 0x70,
    QVM_OP_FREE         = 0x71,
    QVM_OP_LOAD_PTR     = 0x72,
    QVM_OP_STORE_PTR    = 0x73
} qvm_opcode;

typedef struct qvm_instruction {
    qvm_opcode opcode;
    uint8_t    dst;
    uint8_t    src1;
    uint8_t    src2;
    uint16_t   type_id;
    uint32_t   imm32;
    uint8_t    arg_count;
    uint8_t    args[4];
} qvm_instruction;

qvm_error qvm_instruction_decode(const uint8_t *bytecode, uint32_t size, qvm_instruction *ins);
void      qvm_instruction_free(qvm_instruction *ins);
const char* qvm_opcode_name(qvm_opcode op);

// Forward declaration
typedef struct qvm_frame qvm_frame;

// Declaración de qvm_instruction_execute (usada por el runtime como fallback)
qvm_error qvm_instruction_execute(const qvm_instruction *ins, qvm_frame *frame);

#ifdef __cplusplus
}
#endif

#endif // QVM_INSTRUCTION_H

// Funciones auxiliares para testing
qvm_type_id_t qvm_instruction_result_type(const qvm_instruction *ins);
qvm_error qvm_instruction_validate(const qvm_instruction *ins, struct qvm_frame *frame);
