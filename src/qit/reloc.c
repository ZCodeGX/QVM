#include <stdlib.h>
#include <string.h>
#include "qvm/qit.h"

#define MAX_PATCHES 1024
#define MAX_LABELS 2048

typedef enum {
    PATCH_REL32
} patch_kind_t;

typedef struct {
    patch_kind_t kind;
    size_t offset;
    uint32_t target_pc;
} patch_t;

typedef struct {
    uint32_t pc;
    size_t code_offset;
    int defined;
} label_t;

typedef struct {
    patch_t patches[MAX_PATCHES];
    size_t patch_count;
    label_t labels[MAX_LABELS];
    size_t label_count;
} reloc_t;

reloc_t *reloc_create(void) {
    reloc_t *r = calloc(1, sizeof(reloc_t));
    return r;
}

void reloc_destroy(reloc_t *r) {
    free(r);
}

int reloc_define_label(reloc_t *r, uint32_t pc, size_t code_offset) {
    if (r->label_count >= MAX_LABELS) return 0;
    
    // Verificar que no esté ya definida
    for (size_t i = 0; i < r->label_count; i++) {
        if (r->labels[i].pc == pc && r->labels[i].defined) {
            return 0; // Ya definida
        }
    }
    
    // Buscar slot libre o añadir nuevo
    for (size_t i = 0; i < r->label_count; i++) {
        if (r->labels[i].pc == pc) {
            r->labels[i].code_offset = code_offset;
            r->labels[i].defined = 1;
            return 1;
        }
    }
    
    r->labels[r->label_count].pc = pc;
    r->labels[r->label_count].code_offset = code_offset;
    r->labels[r->label_count].defined = 1;
    r->label_count++;
    return 1;
}

int reloc_add_patch_rel32(reloc_t *r, size_t patch_offset, uint32_t target_pc) {
    if (r->patch_count >= MAX_PATCHES) return 0;
    
    r->patches[r->patch_count].kind = PATCH_REL32;
    r->patches[r->patch_count].offset = patch_offset;
    r->patches[r->patch_count].target_pc = target_pc;
    r->patch_count++;
    return 1;
}

int reloc_resolve(reloc_t *r, uint8_t *code, size_t code_size, uint32_t bytecode_size) {
    for (size_t i = 0; i < r->patch_count; i++) {
        patch_t *p = &r->patches[i];
        
        // Validar target
        if (p->target_pc >= bytecode_size) {
            return 0; // Target fuera de rango
        }
        
        // Buscar label
        int found = 0;
        size_t target_offset = 0;
        for (size_t j = 0; j < r->label_count; j++) {
            if (r->labels[j].pc == p->target_pc && r->labels[j].defined) {
                target_offset = r->labels[j].code_offset;
                found = 1;
                break;
            }
        }
        
        if (!found) {
            return 0; // Label no definida
        }
        
        // Validar offset del patch
        if (p->offset + 4 > code_size) {
            return 0;
        }
        
        // Calcular desplazamiento relativo
        int64_t delta = (int64_t)target_offset - (int64_t)(p->offset + 4);
        if (delta > INT32_MAX || delta < INT32_MIN) {
            return 0; // Desplazamiento fuera de rango
        }
        
        // Aplicar patch
        uint32_t value = (uint32_t)(int32_t)delta;
        code[p->offset + 0] = (uint8_t)(value & 0xFF);
        code[p->offset + 1] = (uint8_t)((value >> 8) & 0xFF);
        code[p->offset + 2] = (uint8_t)((value >> 16) & 0xFF);
        code[p->offset + 3] = (uint8_t)((value >> 24) & 0xFF);
    }
    
    return 1;
}
