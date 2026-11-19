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
)

target_include_directories(qvm_core PUBLIC include)
target_compile_options(qvm_core PRIVATE -Wall -Wextra -Wpedantic)