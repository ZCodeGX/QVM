#ifndef QVM_CONFIG_H
#define QVM_CONFIG_H

#include <stdint.h>

#include "qvm/limits.h"

#define QVM_CONFIG_JIT_THRESHOLD  10u
#define QVM_CONFIG_MAX_CALL_DEPTH 1024u

#ifdef __cplusplus
extern "C" {
#endif

typedef enum qvm_mode {
    QVM_MODE_PRODUCTION = 0,
    QVM_MODE_SAFE       = 1,
    QVM_MODE_MIXED      = 2
} qvm_mode;

typedef struct qvm_config {
    qvm_mode    mode;
    uint8_t     low_level;
    uint8_t     verify_on_load;
    uint8_t     enable_debug_metadata;
    uint8_t     _reserved;
    qvm_limits  limits;
} qvm_config;

qvm_config qvm_config_default(void);

#ifdef __cplusplus
}
#endif

#endif /* QVM_CONFIG_H */