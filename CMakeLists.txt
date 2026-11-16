cmake_minimum_required(VERSION 3.16)
project(lab3 C)

set(CMAKE_C_STANDARD 11)

add_executable(lab3
    src/main.c
    src/logger.c
    src/shared_memory.c
)

target_include_directories(lab3 PRIVATE include)