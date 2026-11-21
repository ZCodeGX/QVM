#ifndef QVM_BACKEND_H
#define QVM_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "qvm/error.h"
#include "qvm/loader.h"
#include "qvm/qit.h"

/* Identificadores de arquitectura */
typedef enum {
    QVM_ARCH_UNKNOWN = 0,
    QVM_ARCH_X86_64  = 1,
    QVM_ARCH_AARCH64 = 2,
    QVM_ARCH_RISCV64 = 3,
    QVM_ARCH_INTERP  = 4  /* Fallback universal */
} qvm_arch_t;

/* Identificadores de backend */
typedef enum {
    QVM_BACKEND_INTERP       = 0,  /* Intérprete de referencia */
    QVM_BACKEND_X86_64_SYSV  = 1,  /* x86_64 System V ABI */
    QVM_BACKEND_X86_64_WIN64 = 2,  /* x86_64 Windows ABI (futuro) */
    QVM_BACKEND_AARCH64      = 3,  /* AArch64 (futuro) */
    QVM_BACKEND_RISCV64      = 4   /* RISC-V 64 (futuro) */
} qvm_backend_id_t;

/* Información de un backend */
typedef struct {
    qvm_backend_id_t id;
    const char *name;
    qvm_arch_t arch;
    int available;  /* 1 si está compilado y usable, 0 si no */
} qvm_backend_info_t;

/* Detecta la arquitectura actual */
qvm_arch_t qvm_arch_detect(void);

/* Devuelve el nombre de la arquitectura */
const char *qvm_arch_name(qvm_arch_t arch);

/* Lista todos los backends disponibles */
const qvm_backend_info_t *qvm_backend_list(size_t *count);

/* Selecciona el mejor backend para la arquitectura actual */
qvm_backend_id_t qvm_backend_select(qvm_arch_t arch);

/* Compila usando el backend seleccionado (con fallback automático) */
const qvm_compiled_code *qvm_backend_compile(
    qvm_arch_t arch,
    const qvm_module *module,
    uint32_t function_index,
    qvm_error *err
);

#ifdef __cplusplus
}
#endif

#endif // QVM_BACKEND_H
