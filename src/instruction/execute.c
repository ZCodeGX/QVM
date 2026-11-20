/* QVM instruction execute - stub hasta cerrar secciones 7 y 8. */
#include "qvm/instruction.h"

qvm_error qvm_instruction_execute(const qvm_instruction *ins, qvm_frame *frame) {
    (void)ins; (void)frame;
    return QVM_ERR_INTERNAL;
}