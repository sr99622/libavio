#*******************************************************************************
# libavio/CMakeLists.txt
#
# Copyright (c) 2022, 2024 Stephen Rhodes 
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#******************************************************************************/

cmake_minimum_required(VERSION 3.17)

project(libavio VERSION 3.2.7)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
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

add_subdirectory(pybind11)

pybind11_add_module(avio 
    src/avio.cpp
)

target_link_libraries(avio PRIVATE 
    FFmpeg::FFmpeg
    SDL2::SDL2
)

target_include_directories(avio SYSTEM PUBLIC
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
