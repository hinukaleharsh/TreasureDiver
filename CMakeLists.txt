cmake_minimum_required(VERSION 3.24)
project(TreasureDiver LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED PLATFORM)
    set(PLATFORM Desktop CACHE STRING "Target platform for raylib")
endif()

include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(TreasureDiver
    src/main.cpp
    src/Dive.cpp
    src/Draw.cpp
    src/Globals.cpp
    src/Helpers.cpp
    src/SaveSystem.cpp
)
target_link_libraries(TreasureDiver PRIVATE raylib)

if(PLATFORM STREQUAL "Web")
    set_target_properties(TreasureDiver PROPERTIES
        OUTPUT_NAME index
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
    )
    target_link_options(TreasureDiver PRIVATE
        "-sASSERTIONS=1"
        "-sWASM=1"
        "-sASYNCIFY"
        "-sGL_ENABLE_GET_PROC_ADDRESS=1"
        "-sALLOW_MEMORY_GROWTH=1"
    )
endif()
