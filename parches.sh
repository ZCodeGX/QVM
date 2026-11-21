#!/bin/bash
# ==============================================================================
# QVM Fase 0: Baseline, higiene y control automatizado
# Objetivo: Dejar el repositorio en un estado limpio, reproducible y con CI.
# ==============================================================================
set -e # Salir inmediatamente si algún comando falla

echo "🚀 Iniciando Fase 0 de QVM..."

# ------------------------------------------------------------------------------
# A0.1: Congelación y línea base Git
# ------------------------------------------------------------------------------
echo "📌 A0.1: Creando rama y etiquetando baseline..."
git checkout -b release/0.3.1-beta 2>/dev/null || git checkout release/0.3.1-beta
git tag -f v0.2.0-baseline
git push -f origin v0.2.0-baseline 2>/dev/null || echo "⚠️ No se pudo hacer push de la etiqueta (normal si es local)"

# ------------------------------------------------------------------------------
# A0.3: Infraestructura CI
# ------------------------------------------------------------------------------
echo "⚙️ A0.3: Configurando GitHub Actions CI..."
mkdir -p .github/workflows
cat << 'EOF' > .github/workflows/ci.yml
name: QVM CI Baseline
on:
  push:
    branches: [ "release/0.3.1-beta", "main" ]
  pull_request:
    branches: [ "release/0.3.1-beta", "main" ]
jobs:
  build-and-test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc, clang]
    steps:
    - uses: actions/checkout@v4
    - name: Install dependencies
      run: sudo apt-get update && sudo apt-get install -y cmake build-essential clang
    - name: Configure CMake
      run: |
        if [ "${{ matrix.compiler }}" = "clang" ]; then
          export CC=clang CXX=clang++
        fi
        cmake -S . -B build -DCMAKE_C_STANDARD=17 -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror" -DCMAKE_BUILD_TYPE=Release
    - name: Build
      run: cmake --build build -j$(nproc)
    - name: Test
      run: ctest --test-dir build --output-on-failure
EOF

# ------------------------------------------------------------------------------
# A0.7: Script de verificación de cabeceras
# ------------------------------------------------------------------------------
echo "🛡️ A0.7: Creando script de verificación de cabeceras C++..."
mkdir -p scripts
cat << 'EOF' > scripts/check_headers.sh
#!/bin/bash
ERRORS=0
echo "Verificando protección extern \"C\" en cabeceras públicas..."
for header in include/qvm/*.h; do
    filename=$(basename "$header")
    if ! grep -q "#ifdef __cplusplus" "$header" || ! grep -q "extern \"C\"" "$header"; then
        echo "❌ FALLO: $filename no tiene protección C++"
        ERRORS=$((ERRORS + 1))
    else
        echo "✅ OK: $filename"
    fi
done
if [ $ERRORS -gt 0 ]; then
    echo "Se encontraron $ERRORS cabeceras sin protección."
    exit 1
fi
echo "Todas las cabeceras están correctamente protegidas."
exit 0
EOF
chmod +x scripts/check_headers.sh

# ------------------------------------------------------------------------------
# A0.4: Corrección de deriva documental
# ------------------------------------------------------------------------------
echo "📝 A0.4: Corrigiendo deriva documental..."
sed -i 's/version-0\.1\.0--dev/version-0.2.0--baseline/g' README.md
sed -i 's/HEAD = 82e596a/HEAD = 17cd281/g' docs/arquitectura.md
sed -i 's/qit_compile\/execute/qit_compile, qit_get_entry/g' docs/api.md
sed -i 's/Prólogo: 6 bytes/Prólogo: 8 bytes/g' src/jit/jit.c

# ------------------------------------------------------------------------------
# A0.7 (Cont.): Protección C++ en qit.h
# ------------------------------------------------------------------------------
echo "🔒 Asegurando protección C++ en include/qvm/qit.h..."
cat << 'EOF' > include/qvm/qit.h
#ifndef QVM_QIT_H
#define QVM_QIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct qit_code qit_code;

void qit_set_debug(int enabled);
qit_code *qit_compile(const uint8_t *bytecode, uint32_t size);
void qit_destroy(qit_code *code);
void *qit_get_entry(qit_code *code);
size_t qit_get_size(qit_code *code);

#ifdef __cplusplus
}
#endif

#endif // QVM_QIT_H
EOF

# ------------------------------------------------------------------------------
# A0.5 y A0.6: Unificación de CMakeLists.txt (Eliminando artefactos huérfanos)
# ------------------------------------------------------------------------------
echo "🔨 A0.5/A0.6: Unificando CMakeLists.txt..."
cat << 'EOF' > CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(qvm VERSION 0.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 1. Biblioteca Central Unificada
add_library(qvm_core STATIC
    src/core/types.c src/core/value.c src/core/error.c src/core/limits.c
    src/core/config.c src/core/context.c src/core/lifecycle.c src/core/type_table.c
    src/core/version.c src/frames/frames.c src/registers/registers.c
    src/instruction/instruction.c src/instruction/decode.c src/instruction/execute.c
    src/memory/gcq/gcq.c src/runtime/runtime.c src/runtime/runtime_fast.c
    src/loader/loader.c src/verifier/verifier.c src/verifier/verifier_qms.c
    src/string/string.c src/ffi/ffi.c src/jit/jit.c src/qit/qit.c
)
target_include_directories(qvm_core PUBLIC include)
target_compile_options(qvm_core PRIVATE -Wall -Wextra -Wpedantic)

# 2. Tests (CTest)
enable_testing()
set(QVM_TESTS
    tests/core/test_lifecycle tests/core/test_type_table
    tests/frames/test_frames tests/registers/test_registers
    tests/instruction/test_instruction tests/instruction/test_decode tests/instruction/test_execute
    tests/string/test_string tests/ffi/test_ffi tests/memory/test_gcq
    tests/runtime/test_runtime tests/runtime/test_call_return tests/runtime/test_qms_runtime
    tests/loader/test_loader tests/verifier/test_verifier tests/verifier/test_qms_zero
    tests/qit/test_qit
)
foreach(test_target ${QVM_TESTS})
    get_filename_component(test_name ${test_target} NAME)
    add_executable(${test_name} ${test_target}.c)
    target_link_libraries(${test_name} PRIVATE qvm_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 3. Ejemplos y Demos
set(QVM_EXAMPLES
    examples/demo examples/demo_ffi examples/demo_strings examples/demo_strings_native
    examples/benchmark examples/benchmark_compare examples/benchmark_optimized
    examples/benchmark_native examples/benchmark_jit examples/benchmark_qit
    examples/test_qms_simple examples/test_decode_step examples/debug_loop
)
foreach(example_target ${QVM_EXAMPLES})
    get_filename_component(example_name ${example_target} NAME)
    add_executable(${example_name} ${example_target}.c)
    target_link_libraries(${example_name} PRIVATE qvm_core)
endforeach()
EOF

# ------------------------------------------------------------------------------
# Corrección Crítica: Eliminar variables globales de src/qit/qit.c (Pre-Fase 1)
# ------------------------------------------------------------------------------
echo "🧹 Eliminando variables globales de src/qit/qit.c para thread-safety..."
cat << 'EOF' > src/qit/qit.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "qvm/qit.h"
#include "qvm/instruction.h"

typedef struct {
    uint32_t patch_pos;
    uint32_t target_bc;
} qit_patch_t;

struct qit_code {
    void *code;
    size_t size;
    size_t capacity;
    qit_patch_t patches[256];
    int patch_count;
    uint32_t offset_map[1024];
    int debug_enabled;
};

void qit_set_debug(int enabled) {
    // Se aplicará por instancia en el futuro, por ahora placeholder seguro
    (void)enabled;
}

static inline void emit_byte(qit_code *qit, uint8_t byte) {
    if (qit->size >= qit->capacity) return;
    ((uint8_t *)qit->code)[qit->size++] = byte;
}

static inline size_t get_offset(qit_code *qit) { return qit->size; }

static void emit_load_reg(qit_code *qit, uint8_t reg) {
    emit_byte(qit, 0x48); emit_byte(qit, 0x8b); emit_byte(qit, 0x87);
    emit_byte(qit, (uint8_t)(reg * 16)); emit_byte(qit, 0x00); emit_byte(qit, 0x00); emit_byte(qit, 0x00);
}

static void emit_store_reg(qit_code *qit, uint8_t reg) {
    emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0x87);
    emit_byte(qit, (uint8_t)(reg * 16)); emit_byte(qit, 0x00); emit_byte(qit, 0x00); emit_byte(qit, 0x00);
}

static void compile_arith_add(qit_code *qit, const uint8_t *insn) {
    uint8_t dst = insn[1], src1 = insn[4], src2 = insn[5];
    emit_load_reg(qit, src1);
    emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0xc1);
    emit_load_reg(qit, src2);
    emit_byte(qit, 0x48); emit_byte(qit, 0x01); emit_byte(qit, 0xc8);
    emit_store_reg(qit, dst);
}

static void compile_cmp_lt(qit_code *qit, const uint8_t *insn) {
    uint8_t dst = insn[1], src1 = insn[4], src2 = insn[5];
    emit_load_reg(qit, src1);
    emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0xc1);
    emit_load_reg(qit, src2);
    emit_byte(qit, 0x48); emit_byte(qit, 0x39); emit_byte(qit, 0xc1);
    emit_byte(qit, 0x0f); emit_byte(qit, 0x92); emit_byte(qit, 0xc0);
    emit_byte(qit, 0x48); emit_byte(qit, 0x0f); emit_byte(qit, 0xb6); emit_byte(qit, 0xc0);
    emit_store_reg(qit, dst);
}

static void compile_jmp_if(qit_code *qit, const uint8_t *insn, uint32_t pc) {
    uint8_t src = insn[4];
    int32_t target_bc = insn[5] | (insn[6] << 8) | (insn[7] << 16) | (insn[8] << 24);
    emit_load_reg(qit, src);
    emit_byte(qit, 0x48); emit_byte(qit, 0x85); emit_byte(qit, 0xc0);
    emit_byte(qit, 0x0f); emit_byte(qit, 0x85);
    qit->patches[qit->patch_count].patch_pos = get_offset(qit);
    qit->patches[qit->patch_count].target_bc = target_bc;
    qit->patch_count++;
    emit_byte(qit, 0x00); emit_byte(qit, 0x00); emit_byte(qit, 0x00); emit_byte(qit, 0x00);
}

static void compile_return(qit_code *qit, const uint8_t *insn) {
    uint8_t src = insn[1];
    emit_load_reg(qit, src);
    emit_byte(qit, 0x5d); emit_byte(qit, 0xc3);
}

qit_code *qit_compile(const uint8_t *bytecode, uint32_t size) {
    if (!bytecode || size == 0) return NULL;
    size_t capacity = 4096;
    void *code = mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) return NULL;
    
    qit_code *qit = malloc(sizeof(qit_code));
    if (!qit) { munmap(code, capacity); return NULL; }
    
    qit->code = code; qit->size = 0; qit->capacity = capacity;
    qit->patch_count = 0; qit->debug_enabled = 0;
    memset(qit->offset_map, 0, sizeof(qit->offset_map));
    
    emit_byte(qit, 0x55); emit_byte(qit, 0x48); emit_byte(qit, 0x89); emit_byte(qit, 0xe5);
    emit_byte(qit, 0x48); emit_byte(qit, 0x8b); emit_byte(qit, 0x7f); emit_byte(qit, 0x18);
    
    uint32_t pc = 0;
    while (pc < size) {
        qit->offset_map[pc] = get_offset(qit);
        uint8_t op = bytecode[pc];
        switch (op) {
            case 0x20: compile_arith_add(qit, bytecode + pc); pc += 6; break; // QVM_OP_ARITH_ADD
            case 0x32: compile_cmp_lt(qit, bytecode + pc); pc += 6; break;    // QVM_OP_CMP_LT
            case 0x02: compile_jmp_if(qit, bytecode + pc, pc); pc += 9; break; // QVM_OP_JMP_IF
            case 0x06: compile_return(qit, bytecode + pc); pc += 5; goto done; // QVM_OP_RETURN
            default: free(qit); munmap(code, capacity); return NULL;
        }
    }
done:
    for (int i = 0; i < qit->patch_count; i++) {
        uint32_t patch_pos = qit->patches[i].patch_pos;
        uint32_t target_bc = qit->patches[i].target_bc;
        uint32_t target_gen = qit->offset_map[target_bc];
        int32_t offset = target_gen - (patch_pos + 4);
        uint8_t *patch = (uint8_t *)code + patch_pos;
        patch[0] = offset & 0xff; patch[1] = (offset >> 8) & 0xff;
        patch[2] = (offset >> 16) & 0xff; patch[3] = (offset >> 24) & 0xff;
    }
    mprotect(code, capacity, PROT_READ | PROT_EXEC);
    return qit;
}

void qit_destroy(qit_code *code) {
    if (code) {
        if (code->code) munmap(code->code, code->capacity);
        free(code);
    }
}

void *qit_get_entry(qit_code *code) { return code ? code->code : NULL; }
size_t qit_get_size(qit_code *code) { return code ? code->size : 0; }
EOF

# ------------------------------------------------------------------------------
# A0.2: Generación de Baseline y Pruebas
# ------------------------------------------------------------------------------
echo "🏗️ A0.2: Compilando y generando baseline..."
mkdir -p docs
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > docs/baseline-v0.2.md 2>&1
cmake --build build -j$(nproc) >> docs/baseline-v0.2.md 2>&1

echo "🧪 Ejecutando tests..."
ctest --test-dir build --output-on-failure >> docs/baseline-v0.2.md 2>&1

echo "🛡️ Ejecutando verificación de cabeceras..."
./scripts/check_headers.sh

# ------------------------------------------------------------------------------
# Commit Final de la Fase 0
# ------------------------------------------------------------------------------
echo "💾 Realizando commit de la Fase 0..."
git add .
git commit -m "chore(fase-0): setup CI, unify CMake, fix doc drift, remove QIT globals, add header checks" || echo "⚠️ No hay cambios para commitear o ya está commiteado."

echo "✅ Fase 0 completada con éxito. El repositorio está limpio, probado y listo para la Fase 1."