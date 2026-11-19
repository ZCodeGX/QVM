#include "qvm/limits.h"

/* Defaults conservadores; el embedder puede sobrescribirlos via qvm_config. */
qvm_limits qvm_limits_default(void) {
    qvm_limits l;
    l.max_memory_bytes        = 1024ull * 1024ull * 1024ull;  /* 1 GiB */
    l.max_call_depth          = 1024u;
    l.max_registers_per_frame = 4096u;
    l.max_frame_slots         = 4096u;
    l.max_bytecode_bytes      = 64u * 1024u * 1024u;          /* 64 MiB */
    l.max_constants           = 65536u;
    l.max_functions           = 8192u;
    l.max_modules             = 256u;
    l.max_string_length       = 1u * 1024u * 1024u;           /* 1 MiB */
    l.max_gcq_handles         = 1048576u;
    return l;
}