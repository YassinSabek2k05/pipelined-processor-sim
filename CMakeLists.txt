cmake_minimum_required(VERSION 3.10)
project(pipelined-processor-sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_SOURCE_PATH)

add_executable(program
    src/main.c
    src/loader.c
    src/memory.c
    src/parser.c
)

target_include_directories(program PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(program PRIVATE "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_PATH}\"")
