/* QVM core - ciclo de vida. Transiciones controladas del contexto. */
#include "qvm/lifecycle.h"

extern qvm_state qvm_context_state(const qvm_context *ctx);
extern void      qvm_context_set_state(qvm_context *ctx, qvm_state s);