cmake_minimum_required(VERSION 3.20)
project(libpyle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PYLE_PROFILE_OPS "Enable per-opcode profiling counters (PYLE_PROFILE=1 at runtime)" OFF)

include(FetchContent)

find_package(fmt 12.2.0 QUIET)
if(NOT fmt_FOUND)
    FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.2.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(fmt)
endif()

FetchContent_Declare(unordered_dense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
    GIT_TAG v4.11.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(unordered_dense)

if(TARGET unordered_dense::unordered_dense)
    set(PYLE_UNORDERED_DENSE unordered_dense::unordered_dense)
elseif(TARGET unordered_dense)
    set(PYLE_UNORDERED_DENSE unordered_dense)
else()
    add_library(pyle_unordered_dense INTERFACE)
    target_include_directories(pyle_unordered_dense SYSTEM INTERFACE "${unordered_dense_SOURCE_DIR}/include")
    set(PYLE_UNORDERED_DENSE pyle_unordered_dense)
endif()

file(GLOB_RECURSE PYLE_LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR CMAKE_SYSTEM_NAME STREQUAL "wasm")
    set(PYLE_WASM TRUE)
    list(REMOVE_ITEM PYLE_LIB_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/std/std_proc.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/std/std_process.cpp")
endif()

add_library(libpyle ${PYLE_LIB_SOURCES})
add_library(pyle::libpyle ALIAS libpyle)

target_include_directories(libpyle PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(libpyle PUBLIC cxx_std_17)
target_compile_definitions(libpyle PUBLIC FMT_HEADER_ONLY=1)
target_link_libraries(libpyle PUBLIC fmt::fmt-header-only ${PYLE_UNORDERED_DENSE})

if(PYLE_WASM)
    target_compile_options(libpyle PUBLIC -fwasm-exceptions)
    target_link_options(libpyle PUBLIC -fwasm-exceptions)
endif()

if(PYLE_PROFILE_OPS)
    target_compile_definitions(libpyle PUBLIC PYLE_PROFILE_OPS)
endif()
