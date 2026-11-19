#ifndef QVM_LIMITS_H
#define QVM_LIMITS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qvm_limits {
    uint64_t max_memory_bytes;
    uint32_t max_call_depth;
    uint32_t max_registers_per_frame;
    uint32_t max_frame_slots;
    uint32_t max_bytecode_bytes;
    uint32_t max_constants;
    uint32_t max_functions;
    uint32_t max_modules;
    uint32_t max_string_length;
    uint32_t max_gcq_handles;
} qvm_limits;

qvm_limits qvm_limits_default(void);

#ifdef __cplusplus
}
#endif

#endif /* QVM_LIMITS_H */