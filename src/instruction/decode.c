/* QVM instruction decode - stub hasta cerrar seccion 7 (formato QCB). */
#include "qvm/instruction.h"

qvm_error qvm_instruction_decode(const uint8_t *bytes, uint32_t len, qvm_instruction *out) {
    (void)bytes; (void)len; (void)out;
    return QVM_ERR_INVALID_BYTECODE;
}