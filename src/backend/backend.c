#include "qvm/backend.h"
#include <string.h>

/* Forward declarations de los backends */
extern const qvm_compiled_code *qvm_backend_interp_compile(
    const qvm_module *module, uint32_t function_index, qvm_error *err);
extern const qvm_compiled_code *qvm_backend_x86_64_compile(
    const qvm_module *module, uint32_t function_index, qvm_error *err);

/* Tabla de backends disponibles (se llena en tiempo de compilación) */
static const qvm_backend_info_t backends[] = {
    { QVM_BACKEND_INTERP,       "interp",       QVM_ARCH_INTERP,  1 },
    { QVM_BACKEND_X86_64_SYSV,  "x86_64_sysv",  QVM_ARCH_X86_64,  
#if defined(__x86_64__) || defined(_M_X64)
      1 
#else
      0 
#endif
    },
    { QVM_BACKEND_AARCH64,      "aarch64",      QVM_ARCH_AARCH64, 0 },
    { QVM_BACKEND_RISCV64,      "riscv64",      QVM_ARCH_RISCV64, 0 }
};

static const size_t backend_count = sizeof(backends) / sizeof(backends[0]);

qvm_arch_t qvm_arch_detect(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return QVM_ARCH_X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return QVM_ARCH_AARCH64;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return QVM_ARCH_RISCV64;
#else
    return QVM_ARCH_UNKNOWN;
#endif
}

const char *qvm_arch_name(qvm_arch_t arch) {
    switch (arch) {
        case QVM_ARCH_X86_64:  return "x86_64";
        case QVM_ARCH_AARCH64: return "aarch64";
        case QVM_ARCH_RISCV64: return "riscv64";
        case QVM_ARCH_INTERP:  return "interp";
        default:               return "unknown";
    }
}

const qvm_backend_info_t *qvm_backend_list(size_t *count) {
    if (count) *count = backend_count;
    return backends;
}

qvm_backend_id_t qvm_backend_select(qvm_arch_t arch) {
    /* Buscar el mejor backend disponible para esta arquitectura */
    for (size_t i = 0; i < backend_count; i++) {
        if (backends[i].arch == arch && backends[i].available) {
            return backends[i].id;
        }
    }
    /* Fallback al intérprete */
    return QVM_BACKEND_INTERP;
}

const qvm_compiled_code *qvm_backend_compile(
    qvm_arch_t arch,
    const qvm_module *module,
    uint32_t function_index,
    qvm_error *err)
{
    qvm_backend_id_t backend = qvm_backend_select(arch);
    
    switch (backend) {
        case QVM_BACKEND_X86_64_SYSV:
            return qvm_backend_x86_64_compile(module, function_index, err);
        case QVM_BACKEND_INTERP:
        default:
            return qvm_backend_interp_compile(module, function_index, err);
    }
}
