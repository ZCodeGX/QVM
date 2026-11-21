#include "qvm/os.h"

/* 
 * TODO: Implementar con VirtualAlloc, VirtualProtect, VirtualFree 
 * cuando se añada soporte oficial de Windows al CI.
 * Por ahora, devuelve error para evitar enlazado accidental en POSIX.
 */
qvm_error qvm_os_memory_reserve(void **out_ptr, size_t size) {
    (void)out_ptr; (void)size;
    return QVM_ERR_NOT_IMPLEMENTED;
}

qvm_error qvm_os_memory_protect(void *ptr, size_t size, qvm_os_prot_t prot) {
    (void)ptr; (void)size; (void)prot;
    return QVM_ERR_NOT_IMPLEMENTED;
}

qvm_error qvm_os_memory_release(void *ptr, size_t size) {
    (void)ptr; (void)size;
    return QVM_ERR_NOT_IMPLEMENTED;
}
