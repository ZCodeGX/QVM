#include <stdio.h>
#include <stddef.h>
#include "qvm/qvm.h"

// Necesitamos la definición completa de qvm_frame, que está en frames.c
// Pero podemos inferirla o usar las funciones de acceso.
// Vamos a forzar la definición igual que en frames.c para verificar
struct qvm_frame_debug {
    uint32_t           function_index;
    uint32_t           register_count;
    const struct qvm_frame_debug *caller;
    qvm_type_id_t     *register_types;
    qvm_value         *registers;
};

int main(void) {
    printf("sizeof(qvm_value) = %zu\n", sizeof(qvm_value));
    printf("offsetof(qvm_value, payload) = %zu\n", offsetof(qvm_value, payload));
    printf("offsetof(qvm_value, type_id) = %zu\n", offsetof(qvm_value, type_id));
    
    printf("sizeof(struct qvm_frame_debug) = %zu\n", sizeof(struct qvm_frame_debug));
    printf("offsetof(struct qvm_frame_debug, registers) = %zu\n", offsetof(struct qvm_frame_debug, registers));
    
    return 0;
}
