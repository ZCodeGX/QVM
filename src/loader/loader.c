#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/loader.h"
#include "qvm/verifier.h"

static uint32_t rd32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static qvm_error parse_sections(const uint8_t *data, uint32_t size, qvm_module *module) {
    uint32_t section_count = rd32(data + 12);
    uint32_t offset = 16;

    for (uint32_t i = 0; i < section_count; ++i) {
        if (offset + 8 > size) return QVM_ERR_INVALID_BYTECODE;
        uint32_t section_type = rd32(data + offset);
        uint32_t section_size = rd32(data + offset + 4);
        offset += 8;
        if (section_size > size - offset) return QVM_ERR_INVALID_BYTECODE;

        if (section_type == QCB_SECTION_BYTECODE) {
            module->bytecode      = module->owned_data + offset;
            module->bytecode_size = section_size;
        } else if (section_type == QCB_SECTION_CONSTANTS) {
            uint32_t n = section_size / (uint32_t)sizeof(qvm_value);
            if (n > 0) {
                qvm_value *c = (qvm_value *)malloc(n * sizeof(qvm_value));
                if (!c) return QVM_ERR_OUT_OF_MEMORY;
                memcpy(c, data + offset, n * sizeof(qvm_value));
                module->owned_constants = c;
                module->constants       = c;
                module->constant_count  = n;
            }
        } else if (section_type == QCB_SECTION_FUNCTIONS) {
            module->function_count = section_size / 8u;
        }

        offset += section_size;
    }
    return QVM_OK;
}

qvm_module *qvm_module_load_from_memory(const uint8_t *data, uint32_t size, qvm_context *ctx, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!data || !ctx) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    if (size < 16 || rd32(data) != QCB_MAGIC) {
        if (err) *err = QVM_ERR_INVALID_BYTECODE;
        return NULL;
    }
    if (rd32(data + 4) != QCB_VERSION_1_0) {
        if (err) *err = QVM_ERR_MODULE_INCOMPATIBLE;
        return NULL;
    }

    qvm_module *module = (qvm_module *)calloc(1, sizeof *module);
    if (!module) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    module->owned_data = (uint8_t *)malloc(size);
    if (!module->owned_data) {
        free(module);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    memcpy(module->owned_data, data, size);

    qvm_error perr = parse_sections(module->owned_data, size, module);
    if (perr != QVM_OK) {
        qvm_module_destroy(module);
        if (err) *err = perr;
        return NULL;
    }

    const qvm_config *cfg = qvm_context_config(ctx);
    if (cfg && cfg->verify_on_load) {
        qvm_verifier_result vr;
        qvm_error verr = qvm_verify_module(module, ctx, &vr);
        if (verr != QVM_OK) {
            qvm_module_destroy(module);
            if (err) *err = QVM_ERR_VERIFICATION_FAILED;
            return NULL;
        }
    }

    return module;
}

qvm_module *qvm_module_load_from_file(const char *path, qvm_context *ctx, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!path || !ctx) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        if (err) *err = QVM_ERR_IO;
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); if (err) *err = QVM_ERR_IO; return NULL; }
    long file_size = ftell(f);
    if (file_size <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        if (err) *err = QVM_ERR_IO;
        return NULL;
    }

    uint8_t *data = (uint8_t *)malloc((size_t)file_size);
    if (!data) {
        fclose(f);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    size_t got = fread(data, 1, (size_t)file_size, f);
    fclose(f);
    if (got != (size_t)file_size) {
        free(data);
        if (err) *err = QVM_ERR_IO;
        return NULL;
    }

    qvm_module *module = qvm_module_load_from_memory(data, (uint32_t)file_size, ctx, err);
    free(data);
    return module;
}

void qvm_module_destroy(qvm_module *module) {
    if (!module) return;
    free(module->owned_constants);
    free(module->owned_data);
    free(module);
}
