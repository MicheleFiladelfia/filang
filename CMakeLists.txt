cmake_minimum_required(VERSION 3.20)
project(filang C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

# Check Homebrew readline on macOS if available
if (APPLE)
    if (EXISTS "/opt/homebrew/opt/readline")
        include_directories("/opt/homebrew/opt/readline/include")
        link_directories("/opt/homebrew/opt/readline/lib")
    elseif (EXISTS "/usr/local/opt/readline")
        include_directories("/usr/local/opt/readline/include")
        link_directories("/usr/local/opt/readline/lib")
    endif()
endif()

add_executable(filang
    main.c
    scanner.c
    scanner.h
    vm.c
    vm.h
    token.h
    value.c
    value.h
    chunk.c
    chunk.h
    memory.c
    memory.h
    compiler.c
    compiler.h
    hashmap.c
    hashmap.h
    strings.c
    strings.h
    disassembler.c
    disassembler.h
)

find_library(READLINE_LIBRARY NAMES readline)
if (READLINE_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${READLINE_LIBRARY})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE readline)
endif()

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

