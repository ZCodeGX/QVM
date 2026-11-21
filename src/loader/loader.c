#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qvm/loader.h"
#include "qvm/string.h"
#include "qvm/verifier.h"

static uint32_t rd32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static uint16_t rd16(const uint8_t *b) {
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

static qvm_error parse_constants(const uint8_t *data, uint32_t size, qvm_module *module) {
    uint32_t offset = 0;
    uint32_t count = 0;
    
    while (offset < size) {
        if (offset + 6 > size) return QVM_ERR_INVALID_BYTECODE;
        uint16_t type_id = rd16(data + offset);
        offset += 2;
        
        if (type_id == QVM_TYPE_STRING) {
            if (offset + 4 > size) return QVM_ERR_INVALID_BYTECODE;
            uint32_t len = rd32(data + offset);
            offset += 4 + len;
        } else {
            offset += 8;
        }
        count++;
    }
    
    if (count == 0) return QVM_OK;
    
    qvm_value *constants = (qvm_value *)malloc(count * sizeof(qvm_value));
    if (!constants) return QVM_ERR_OUT_OF_MEMORY;
    
    offset = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t type_id = rd16(data + offset);
        offset += 2;
        
        constants[i].type_id = type_id;
        
        if (type_id == QVM_TYPE_STRING) {
            uint32_t len = rd32(data + offset);
            offset += 4;
            
            qvm_error err;
            qvm_string *str = qvm_string_create((const char *)(data + offset), len, &err);
            if (!str) {
                for (uint32_t j = 0; j < i; ++j) {
                    if (constants[j].type_id == QVM_TYPE_STRING) {
                        qvm_string *s;
                        memcpy(&s, &constants[j].payload, sizeof(qvm_string *));
                        qvm_string_release(s);
                    }
                }
                free(constants);
                return err;
            }
            
            memset(&constants[i].payload, 0, sizeof(constants[i].payload));
            memcpy(&constants[i].payload, &str, sizeof(qvm_string *));
            offset += len;
        } else {
            if (offset + 8 > size) {
                free(constants);
                return QVM_ERR_INVALID_BYTECODE;
            }
            memcpy(&constants[i].payload, data + offset, 8);
            offset += 8;
        }
    }
    
    module->owned_constants = constants;
    module->constants = constants;
    module->constant_count = count;
    return QVM_OK;
}

static qvm_error parse_functions(const uint8_t *data, uint32_t size, qvm_module *module) {
    // 1. Contar funciones iterando (porque el tamaño es variable)
    uint32_t offset = 0;
    uint32_t count = 0;
    
    while (offset + 10 <= size) {
        uint16_t local_count = rd16(data + offset + 6);
        uint32_t func_size = 10 + (local_count * 2);
        if (offset + func_size > size) return QVM_ERR_INVALID_BYTECODE;
        offset += func_size;
        count++;
    }
    
    if (count == 0) return QVM_OK;
    
    // 2. Asignar memoria y parsear
    qvm_function *functions = (qvm_function *)calloc(count, sizeof(qvm_function));
    if (!functions) return QVM_ERR_OUT_OF_MEMORY;
    
    offset = 0;
    for (uint32_t i = 0; i < count; ++i) {
        functions[i].bytecode_offset = rd32(data + offset);
        functions[i].param_count     = rd16(data + offset + 4);
        functions[i].local_count     = rd16(data + offset + 6);
        functions[i].return_type     = rd16(data + offset + 8);
        
        if (functions[i].local_count > 0) {
            functions[i].local_types = (qvm_type_id_t *)malloc(functions[i].local_count * sizeof(qvm_type_id_t));
            if (!functions[i].local_types) {
                // Limpieza en caso de error
                for (uint32_t j = 0; j < i; ++j) free((void *)functions[j].local_types);
                free(functions);
                return QVM_ERR_OUT_OF_MEMORY;
            }
            for (uint16_t j = 0; j < functions[i].local_count; ++j) {
                functions[i].local_types[j] = rd16(data + offset + 10 + (j * 2));
            }
        }
        
        offset += 10 + (functions[i].local_count * 2);
    }
    
    module->functions = functions;
    module->function_count = count;
    return QVM_OK;
}

qvm_module *qvm_module_load_from_memory(const uint8_t *data, uint32_t size, qvm_context *ctx, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!data || !ctx) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    
    if (size < 16) {
        if (err) *err = QVM_ERR_INVALID_BYTECODE;
        return NULL;
    }
    
    uint32_t magic = rd32(data);
    if (magic != QCB_MAGIC) {
        if (err) *err = QVM_ERR_INVALID_BYTECODE;
        return NULL;
    }
    
    uint32_t version = rd32(data + 4);
    if (version != QCB_VERSION_1_0) {
        if (err) *err = QVM_ERR_MODULE_INCOMPATIBLE;
        return NULL;
    }
    
    uint32_t section_count = rd32(data + 12);
    uint32_t offset = 16;
    
    qvm_module *module = (qvm_module *)calloc(1, sizeof(qvm_module));
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
    module->version = version;

    for (uint32_t i = 0; i < section_count; ++i) {
        if (offset + 8 > size) {
            qvm_module_destroy(module);
            if (err) *err = QVM_ERR_INVALID_BYTECODE;
            return NULL;
        }
        
        uint32_t section_type = rd32(module->owned_data + offset);
        uint32_t section_size = rd32(module->owned_data + offset + 4);
        offset += 8;
        
        if (offset + section_size > size) {
            qvm_module_destroy(module);
            if (err) *err = QVM_ERR_INVALID_BYTECODE;
            return NULL;
        }
        
        qvm_error parse_err = QVM_OK;
        
        switch (section_type) {
            case QCB_SECTION_CONSTANTS:
                parse_err = parse_constants(module->owned_data + offset, section_size, module);
                break;
            case QCB_SECTION_FUNCTIONS:
                parse_err = parse_functions(module->owned_data + offset, section_size, module);
                break;
            case QCB_SECTION_BYTECODE:
                module->bytecode = module->owned_data + offset;
                module->bytecode_size = section_size;
                break;
        }
        
        if (parse_err != QVM_OK) {
            qvm_module_destroy(module);
            if (err) *err = parse_err;
            return NULL;
        }
        
        offset += section_size;
    }
    
    const qvm_config *config = qvm_context_config(ctx);
    if (config && config->verify_on_load) {
        qvm_verifier_result result;
        qvm_error verify_err = qvm_verify_module(module, ctx, &result);
        if (verify_err != QVM_OK) {
            qvm_module_destroy(module);
            if (err) *err = verify_err;
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
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
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
    
    size_t read = fread(data, 1, (size_t)file_size, f);
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
    if (!module) return;
    
    if (module->owned_constants) {
        for (uint32_t i = 0; i < module->constant_count; ++i) {
            if (module->owned_constants[i].type_id == QVM_TYPE_STRING) {
                qvm_string *str;
                memcpy(&str, &module->owned_constants[i].payload, sizeof(qvm_string *));
                qvm_string_release(str);
            }
        }
        free(module->owned_constants);
    }
    
    if (module->functions) {
        for (uint32_t i = 0; i < module->function_count; ++i) {
            free((void *)module->functions[i].local_types);
        }
        free(module->functions);
    }
    
    free(module->owned_data);
    free(module);
}
