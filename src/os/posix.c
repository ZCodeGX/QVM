#include "qvm/os.h"
#include <sys/mman.h>
#include <errno.h>

qvm_error qvm_os_memory_reserve(void **out_ptr, size_t size) {
    if (!out_ptr || size == 0) return QVM_ERR_INVALID_ARGUMENT;
    
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return QVM_ERR_OUT_OF_MEMORY;
    }
    *out_ptr = ptr;
    return QVM_OK;
}

qvm_error qvm_os_memory_protect(void *ptr, size_t size, qvm_os_prot_t prot) {
    if (!ptr || size == 0) return QVM_ERR_INVALID_ARGUMENT;
    
    int p = PROT_NONE;
    if (prot & QVM_OS_PROT_READ)  p |= PROT_READ;
    if (prot & QVM_OS_PROT_WRITE) p |= PROT_WRITE;
    if (prot & QVM_OS_PROT_EXEC)  p |= PROT_EXEC;
    
    if (mprotect(ptr, size, p) != 0) {
        return QVM_ERR_INVALID_ARGUMENT; // Podría mapearse a errno específico
    }
    return QVM_OK;
}

qvm_error qvm_os_memory_release(void *ptr, size_t size) {
    if (!ptr || size == 0) return QVM_ERR_INVALID_ARGUMENT;
    
    if (munmap(ptr, size) != 0) {
        return QVM_ERR_INVALID_ARGUMENT;
    }
    return QVM_OK;
}
