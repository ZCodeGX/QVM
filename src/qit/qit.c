#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include "qvm/qit.h"
#include "qvm/instruction.h"
#include "qvm/value.h"

/* ABI: el stride de registros debe coincidir con sizeof(qvm_value). */
_Static_assert(sizeof(qvm_value) == 16,
               "ABI QIT: qvm_value debe medir 16 bytes (stride de registros)");

/* Límites provisionales. Fase 2 (reloc.c + codebuf.c) los elimina. */
#define QIT_MAX_BC      1024u
#define QIT_MAX_PATCHES 256u
#define QIT_CAPACITY    4096u

/* Solo diagnóstico. El estado de compilación es por instancia. */
static atomic_int qit_debug_flag = 0;

typedef struct { uint32_t patch_pos; uint32_t target_bc; } qit_patch_t;

struct qit_code {
    void   *code;
    size_t  size;
    size_t  capacity;
    int     overflow;
    int     patch_count;
    qit_patch_t patches[QIT_MAX_PATCHES];
    uint32_t    offset_map[QIT_MAX_BC];
    uint8_t     mapped[QIT_MAX_BC];
};

void qit_set_debug(int enabled) { atomic_store(&qit_debug_flag, enabled ? 1 : 0); }

static void emit_byte(qit_code *q, uint8_t b) {
    if (q->size >= q->capacity) { q->overflow = 1; return; }
    ((uint8_t *)q->code)[q->size++] = b;
}
static void emit32le(qit_code *q, uint32_t v) {
    emit_byte(q, (uint8_t)(v      )); emit_byte(q, (uint8_t)(v >>  8));
    emit_byte(q, (uint8_t)(v >> 16)); emit_byte(q, (uint8_t)(v >> 24));
}

/* disp32 completo: sin truncar reg*16 a uint8_t (QVM-008). */
static void emit_load_reg(qit_code *q, uint8_t reg) {
    emit_byte(q, 0x48); emit_byte(q, 0x8b); emit_byte(q, 0x87);
    emit32le(q, (uint32_t)reg * 16u);
}
static void emit_store_reg(qit_code *q, uint8_t reg) {
    emit_byte(q, 0x48); emit_byte(q, 0x89); emit_byte(q, 0x87);
    emit32le(q, (uint32_t)reg * 16u);
}

static void compile_add(qit_code *q, const uint8_t *in) {
    emit_load_reg(q, in[4]);
    emit_byte(q, 0x48); emit_byte(q, 0x89); emit_byte(q, 0xc1);
    emit_load_reg(q, in[5]);
    emit_byte(q, 0x48); emit_byte(q, 0x01); emit_byte(q, 0xc8);
    emit_store_reg(q, in[1]);
}
static void compile_cmp_lt(qit_code *q, const uint8_t *in) {
    emit_load_reg(q, in[4]);
    emit_byte(q, 0x48); emit_byte(q, 0x89); emit_byte(q, 0xc1);
    emit_load_reg(q, in[5]);
    emit_byte(q, 0x48); emit_byte(q, 0x39); emit_byte(q, 0xc1);
    emit_byte(q, 0x0f); emit_byte(q, 0x92); emit_byte(q, 0xc0); /* setb: unsigned */
    emit_byte(q, 0x48); emit_byte(q, 0x0f); emit_byte(q, 0xb6); emit_byte(q, 0xc0);
    emit_store_reg(q, in[1]);
}
static int compile_jmp_if(qit_code *q, const uint8_t *in) {
    uint32_t target = (uint32_t)in[5] | ((uint32_t)in[6] << 8)
                    | ((uint32_t)in[7] << 16) | ((uint32_t)in[8] << 24);
    if (q->patch_count >= (int)QIT_MAX_PATCHES) return 0;
    emit_load_reg(q, in[4]);
    emit_byte(q, 0x48); emit_byte(q, 0x85); emit_byte(q, 0xc0);
    emit_byte(q, 0x0f); emit_byte(q, 0x85);
    q->patches[q->patch_count].patch_pos = (uint32_t)q->size;
    q->patches[q->patch_count].target_bc = target;
    q->patch_count++;
    emit32le(q, 0);
    return 1;
}
static void compile_return(qit_code *q, const uint8_t *in) {
    /* Semántica canónica: RETURN devuelve src1 (byte 4), igual que el intérprete. */
    emit_load_reg(q, in[4]);
    emit_byte(q, 0x5d);
    emit_byte(q, 0xc3);
}

qit_code *qit_compile(const uint8_t *bc, uint32_t size) {
    if (!bc || size == 0 || size > QIT_MAX_BC) return NULL;

    void *mem = mmap(NULL, QIT_CAPACITY, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) return NULL;

    qit_code *q = calloc(1, sizeof *q);
    if (!q) { munmap(mem, QIT_CAPACITY); return NULL; }
    q->code = mem; q->capacity = QIT_CAPACITY;

    /* push rbp; mov rbp,rsp; mov rdi,[rdi+24] */
    emit_byte(q, 0x55);
    emit_byte(q, 0x48); emit_byte(q, 0x89); emit_byte(q, 0xe5);
    emit_byte(q, 0x48); emit_byte(q, 0x8b); emit_byte(q, 0x7f); emit_byte(q, 0x18);

    int saw_return = 0;
    uint32_t pc = 0;
    while (pc < size) {
        uint32_t need;
        switch ((qvm_opcode)bc[pc]) {
            case QVM_OP_ARITH_ADD: need = 6; break;
            case QVM_OP_CMP_LT:    need = 6; break;
            case QVM_OP_JMP_IF:    need = 9; break;
            case QVM_OP_RETURN:    need = 5; break;
            default: goto fail;                 /* opcode no soportado */
        }
        if ((uint64_t)pc + need > size) goto fail;   /* QVM-004: lectura OOB */

        q->offset_map[pc] = (uint32_t)q->size;
        q->mapped[pc] = 1;

        switch ((qvm_opcode)bc[pc]) {
            case QVM_OP_ARITH_ADD: compile_add(q, bc + pc);    break;
            case QVM_OP_CMP_LT:    compile_cmp_lt(q, bc + pc); break;
            case QVM_OP_JMP_IF:    if (!compile_jmp_if(q, bc + pc)) goto fail; break;
            case QVM_OP_RETURN:    compile_return(q, bc + pc); saw_return = 1; break;
            default: goto fail;
        }
        if (q->overflow) goto fail;             /* QVM-002: nunca devolver código truncado */
        pc += need;
        if (saw_return) break;
    }

    /* Sin RETURN no hay epílogo: nunca dejar caer la ejecución en bytes cero. */
    if (!saw_return) goto fail;

    for (int i = 0; i < q->patch_count; i++) {
        uint32_t pos = q->patches[i].patch_pos;
        uint32_t tgt = q->patches[i].target_bc;
        if (tgt >= size || !q->mapped[tgt]) goto fail;   /* QVM-005/006: target no emitido */
        if ((uint64_t)pos + 4 > q->size) goto fail;      /* QVM-003 */
        int64_t delta = (int64_t)q->offset_map[tgt] - (int64_t)(pos + 4);
        if (delta > INT32_MAX || delta < INT32_MIN) goto fail;
        uint8_t *p = (uint8_t *)q->code + pos;
        uint32_t v = (uint32_t)(int32_t)delta;
        p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
        if (atomic_load(&qit_debug_flag))
            fprintf(stderr, "QIT: patch %u -> bc %u (gen %u, rel %d)\n",
                    pos, tgt, q->offset_map[tgt], (int)delta);
    }

    if (atomic_load(&qit_debug_flag)) {
        fprintf(stderr, "QIT: %zu bytes emitidos\n", q->size);
        for (size_t i = 0; i < q->size; i++) {
            fprintf(stderr, "%02x ", ((uint8_t *)q->code)[i]);
            if ((i + 1) % 16 == 0) fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }

    if (mprotect(q->code, q->capacity, PROT_READ | PROT_EXEC) != 0) goto fail; /* QVM-012 */
    return q;

fail:
    munmap(mem, QIT_CAPACITY);
    free(q);
    return NULL;
}

void qit_destroy(qit_code *c) {
    if (!c) return;
    if (c->code) munmap(c->code, c->capacity);
    free(c);
}
void  *qit_get_entry(qit_code *c) { return c ? c->code : NULL; }
size_t qit_get_size (qit_code *c) { return c ? c->size : 0; }
