#ifndef QVM_QCB_H
#define QVM_QCB_H

#include <stdint.h>
#include "qvm/error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QCB_MAGIC         ((uint32_t)0x524D4151u)
#define QCB_VERSION_1_0   ((uint32_t)0x00000001u)

#define QCB_FLAG_REQUIRE_SAFE   ((uint32_t)0x00000001u)
#define QCB_FLAG_DEBUG_METADATA ((uint32_t)0x00000002u)

#define QCB_SECTION_TYPES     ((uint32_t)0x01u)
#define QCB_SECTION_CONSTANTS ((uint32_t)0x02u)
#define QCB_SECTION_FUNCTIONS ((uint32_t)0x03u)
#define QCB_SECTION_BYTECODE  ((uint32_t)0x04u)

typedef struct qcb_header {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t section_count;
} qcb_header;

typedef struct qcb_section_header {
    uint32_t type_id;
    uint32_t size_bytes;
} qcb_section_header;

#ifdef __cplusplus
}
#endif

#endif
