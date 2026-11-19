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
    src/core/version.c
    src/frames/frames.c
    src/registers/registers.c
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