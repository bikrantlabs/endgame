cmake_minimum_required(VERSION 3.20)
project(endgame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(endgame ${SOURCES})

target_link_libraries(endgame PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
)