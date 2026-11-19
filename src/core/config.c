#include <string.h>

#include "qvm/config.h"

qvm_config qvm_config_default(void) {
    qvm_config c;
    memset(&c, 0, sizeof c);
    c.mode                  = QVM_MODE_SAFE;
    c.low_level             = 0;
    c.verify_on_load        = 1;
    c.enable_debug_metadata = 1;
    c.limits                = qvm_limits_default();
    return c;
}