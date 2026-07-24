cmake_minimum_required(VERSION 3.20)
project(endgame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")

# ── Embed book.bin into the binary ──────────────────────────────────────
if(EXISTS "${CMAKE_SOURCE_DIR}/book.bin")
    message(STATUS "Embedding book.bin into the binary")

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/generate_book.py"
                "${CMAKE_SOURCE_DIR}/book.bin" "${CMAKE_BINARY_DIR}/book_data.cpp"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    list(APPEND SOURCES "${CMAKE_BINARY_DIR}/book_data.cpp")
    add_definitions(-DEMBED_BOOK)
else()
    message(STATUS "book.bin not found — no embedded book")
endif()

add_executable(endgame ${SOURCES})

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
