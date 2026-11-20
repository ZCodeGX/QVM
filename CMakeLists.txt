cmake_minimum_required(VERSION 3.16)
project(qvm C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(qvm_core STATIC
    src/core/types.c
    src/core/value.c
    src/core/error.c
    src/core/limits.c
    src/core/config.c
    src/core/context.c
    src/core/lifecycle.c
    src/core/type_table.c
    src/core/version.c
    src/frames/frames.c
    src/registers/registers.c
    src/instruction/instruction.c
    src/instruction/decode.c
    src/instruction/execute.c
    src/memory/gcq/gcq.c
    src/runtime/runtime.c
    src/loader/loader.c
    src/verifier/verifier.c
    src/string/string.c
    src/ffi/ffi.c
)

target_include_directories(qvm_core PUBLIC include)
target_compile_options(qvm_core PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()

add_executable(test_lifecycle tests/core/test_lifecycle.c)
target_link_libraries(test_lifecycle qvm_core)
add_test(NAME lifecycle COMMAND test_lifecycle)

add_executable(test_frames tests/frames/test_frames.c)
target_link_libraries(test_frames qvm_core)
add_test(NAME frames COMMAND test_frames)

add_executable(test_registers tests/registers/test_registers.c)
target_link_libraries(test_registers qvm_core)
add_test(NAME registers COMMAND test_registers)

add_executable(test_instruction tests/instruction/test_instruction.c)
target_link_libraries(test_instruction qvm_core)
add_test(NAME instruction COMMAND test_instruction)

add_executable(test_decode tests/instruction/test_decode.c)
target_link_libraries(test_decode qvm_core)
add_test(NAME decode COMMAND test_decode)

add_executable(test_execute tests/instruction/test_execute.c)
target_link_libraries(test_execute qvm_core)
add_test(NAME execute COMMAND test_execute)

add_executable(test_type_table tests/core/test_type_table.c)
target_link_libraries(test_type_table qvm_core)
add_test(NAME type_table COMMAND test_type_table)

add_executable(test_string tests/string/test_string.c)
target_link_libraries(test_string qvm_core)
add_test(NAME string COMMAND test_string)

add_executable(test_ffi tests/ffi/test_ffi.c)
target_link_libraries(test_ffi qvm_core)
add_test(NAME ffi COMMAND test_ffi)

add_executable(test_gcq tests/memory/test_gcq.c)
target_link_libraries(test_gcq qvm_core)
add_test(NAME gcq COMMAND test_gcq)

add_executable(test_runtime tests/runtime/test_runtime.c)
target_link_libraries(test_runtime qvm_core)
add_test(NAME runtime COMMAND test_runtime)

add_executable(test_call_return tests/runtime/test_call_return.c)
target_link_libraries(test_call_return qvm_core)
add_test(NAME call_return COMMAND test_call_return)

add_executable(test_loader tests/loader/test_loader.c)
target_link_libraries(test_loader qvm_core)
add_test(NAME loader COMMAND test_loader)

add_executable(test_verifier tests/verifier/test_verifier.c)
target_link_libraries(test_verifier qvm_core)
add_test(NAME verifier COMMAND test_verifier)

add_executable(demo examples/demo.c)
target_link_libraries(demo qvm_core)

add_executable(demo_ffi examples/demo_ffi.c)
target_link_libraries(demo_ffi qvm_core)

add_executable(demo_strings examples/demo_strings.c)
target_link_libraries(demo_strings qvm_core)

add_executable(demo_strings_native examples/demo_strings_native.c)
target_link_libraries(demo_strings_native qvm_core)

add_executable(benchmark examples/benchmark.c)
target_link_libraries(benchmark qvm_core)

