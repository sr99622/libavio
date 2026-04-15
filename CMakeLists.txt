cmake_minimum_required(VERSION 3.17)

project(libavio VERSION 3.2.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options(/EHsc /MT)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

add_definitions(-w)

find_package(FFmpeg REQUIRED)
find_package(SDL2 REQUIRED)

set(PYBIND11_FINDPYTHON ON)
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(avio
    src/avio.cpp
)

target_link_libraries(avio PRIVATE
    FFmpeg::FFmpeg
    SDL2::SDL2
)

target_include_directories(avio PRIVATE
    include
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("-- Setting run_path for Linux binaries")
    set_target_properties(avio PROPERTIES
        BUILD_RPATH "$ORIGIN"
        BUILD_RPATH_USE_ORIGIN TRUE
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_ORIGIN TRUE
    )
endif()

install(TARGETS avio
    LIBRARY DESTINATION avio
    RUNTIME DESTINATION avio
    ARCHIVE DESTINATION avio
)
