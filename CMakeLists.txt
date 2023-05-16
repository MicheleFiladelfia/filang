cmake_minimum_required(VERSION 3.24)
project(filang C)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-O2")

add_executable(filang main.c scanner.c scanner.h vm.c vm.h token.h value.c value.h chunk.c chunk.h memory.c memory.h compiler.c compiler.h hashmap.c hashmap.h strings.c strings.h disassembler.c disassembler.h)
target_link_libraries(${PROJECT_NAME} m)
target_link_libraries(${PROJECT_NAME} /usr/lib/x86_64-linux-gnu/libreadline.so)
