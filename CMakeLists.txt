cmake_minimum_required(
    VERSION 3.15
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # to generate compile_commands.json

project(toy_cpu
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/build/bin)

file(GLOB_RECURSE PROJECT_SOURCES
    "${PROJECT_ROOT}/src/*.cpp"
)

add_executable(toy_cpu
    ${PROJECT_SOURCES}
)

target_include_directories(toy_cpu
    PRIVATE
        ${PROJECT_ROOT}/include
)

add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/toy_cpu
    DEPENDS toy_cpu
)