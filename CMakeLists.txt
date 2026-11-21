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
