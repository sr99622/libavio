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

project(libavio VERSION 3.2.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")

# pass current dir in from host when building in virtual env
set (MY_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
if (DEFINED ENV{CMAKE_CURRENT_SOURCE_DIR})
    set(MY_CURRENT_DIR $ENV{CMAKE_CURRENT_SOURCE_DIR})
endif()
file(TO_CMAKE_PATH "${MY_CURRENT_DIR}" MY_DIR_VAR)

list(PREPEND CMAKE_MODULE_PATH ${MY_DIR_VAR}/cmake)

if(WIN32)
    list(PREPEND CMAKE_MODULE_PATH ${MY_DIR_VAR}/cmake-win)
    add_compile_options(/EHsc /MT)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

add_definitions(-w)

find_package(FFmpeg REQUIRED)
find_package(SDL2 REQUIRED)

if (NOT WITHOUT_LIBS)
    add_library(libavio SHARED
        src/Clock.cpp
        src/Decoder.cpp
        src/Display.cpp
        src/Encoder.cpp
        src/Exception.cpp
        src/Filter.cpp
        src/Frame.cpp
        src/Packet.cpp
        src/Pipe.cpp
        src/Player.cpp
        src/Reader.cpp
        src/Writer.cpp
    )

    if (WIN32)
        target_link_libraries(libavio PUBLIC
            FFmpeg::FFmpeg
            SDL2::SDL2
        )
    else()
        target_link_libraries(libavio PUBLIC
            FFmpeg::FFmpeg
            SDL2::SDL2
            pthread
        )
        set_target_properties(libavio PROPERTIES
            OUTPUT_NAME avio
            SOVERSION 1
        )
    endif()

    target_include_directories(libavio PUBLIC
        include
        ${FFMPEG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
    )
endif()

if (NOT WITHOUT_PYTHON)
    add_subdirectory(pybind11)

    pybind11_add_module(avio 
        src/avio.cpp
        src/Clock.cpp
        src/Decoder.cpp
        src/Display.cpp
        src/Encoder.cpp
        src/Exception.cpp
        src/Filter.cpp
        src/Frame.cpp
        src/Packet.cpp
        src/Pipe.cpp
        src/Player.cpp
        src/Reader.cpp
        src/Writer.cpp
    )
                    
    target_link_libraries(avio PRIVATE 
        FFmpeg::FFmpeg
        SDL2::SDL2
    )

    target_include_directories(avio SYSTEM PUBLIC
        include
    )
endif()

IF (BUILD_TEST)
    add_executable(avio-test
        test/avio-test.cpp
    )

    target_link_libraries(avio-test PRIVATE
        libavio
    )

    target_include_directories(avio-test PUBLIC
        include
    )

endif()