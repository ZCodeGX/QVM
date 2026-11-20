#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/loader.h"

static uint32_t rd32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static qvm_error validate_header(const uint8_t *data, uint32_t size) {
    if (size < sizeof(qcb_header)) return QVM_ERR_INVALID_BYTECODE;
    uint32_t magic = rd32(data);
    if (magic != QCB_MAGIC) return QVM_ERR_INVALID_BYTECODE;
    uint32_t version = rd32(data + 4);
    if (version != QCB_VERSION_1_0) return QVM_ERR_MODULE_INCOMPATIBLE;
    return QVM_OK;
}

static qvm_error parse_sections(const uint8_t *data, uint32_t size, qvm_module *module) {
    if (size < 16) return QVM_ERR_INVALID_BYTECODE;
    uint32_t section_count = rd32(data + 12);
    uint32_t offset = 16;
    
    for (uint32_t i = 0; i < section_count; ++i) {
        if (offset + 8 > size) return QVM_ERR_INVALID_BYTECODE;
        uint32_t section_type = rd32(data + offset);
        uint32_t section_size = rd32(data + offset + 4);
        offset += 8;
        
        if (offset + section_size > size) return QVM_ERR_INVALID_BYTECODE;
        
        if (section_type == QCB_SECTION_BYTECODE) {
            module->bytecode = data + offset;
            module->bytecode_size = section_size;
        } else if (section_type == QCB_SECTION_CONSTANTS) {
            module->constants = (const qvm_value *)(data + offset);
            module->constant_count = section_size / sizeof(qvm_value);
        } else if (section_type == QCB_SECTION_FUNCTIONS) {
            module->function_count = section_size / 8;
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
    
    qvm_error verr = validate_header(data, size);
    if (verr != QVM_OK) {
        if (err) *err = verr;
        return NULL;
    }
    
    qvm_module *module = (qvm_module *)calloc(1, sizeof *module);
    if (!module) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    verr = parse_sections(data, size, module);
    if (verr != QVM_OK) {
        free(module);
        if (err) *err = verr;
        return NULL;
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
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(f);
        if (err) *err = QVM_ERR_IO;
        return NULL;
    }
    
    uint8_t *data = (uint8_t *)malloc(file_size);
    if (!data) {
        fclose(f);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    size_t read = fread(data, 1, file_size, f);
    fclose(f);
    
    if (read != (size_t)file_size) {
        free(data);
        if (err) *err = QVM_ERR_IO;
        return NULL;
    }
    
    qvm_module *module = qvm_module_load_from_memory(data, (uint32_t)file_size, ctx, err);
    free(data);
    return module;
}

void qvm_module_destroy(qvm_module *module) {
    free(module);
}
