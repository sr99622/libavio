#*******************************************************************************
# libavio/CMakeLists.txt
#
# Copyright (c) 2022 Stephen Rhodes 
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

project(libavio VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(WIN32)
    add_compile_options("/EHsc")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

find_package(FFmpeg REQUIRED)
find_package(SDL2 REQUIRED)

add_library(avio SHARED
    src/Clock.cpp
    src/Decoder.cpp
    src/Display.cpp
    src/Encoder.cpp
    src/Exception.cpp
    src/Filter.cpp
    src/Frame.cpp
    src/Pipe.cpp
    src/Reader.cpp
    src/Writer.cpp
)

if (WIN32)
    target_link_libraries(avio PUBLIC
        FFmpeg::FFmpeg
        SDL2::SDL2
    )
else()
    target_link_libraries(avio PUBLIC
        FFmpeg::FFmpeg
        SDL2::SDL2
        pthread
    )
endif()

set_target_properties(avio PROPERTIES
    SOVERSION 1
)

target_include_directories(avio PUBLIC
    include
    ${FFMPEG_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)

if (BUILD_SAMPLE)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)

    add_executable(sample_cmd
        sample_cmd/main.cpp
    )

    target_link_libraries(sample_cmd PRIVATE
        avio
    )

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(sample_gui
        sample_gui/src/glwidget.cpp
        sample_gui/src/main.cpp
        sample_gui/src/mainwindow.cpp
        sample_gui/include/glwidget.h
        sample_gui/include/mainwindow.h
    )

    target_link_libraries(sample_gui PRIVATE
        Qt5::Widgets
        avio
    )

    target_include_directories(sample_gui PUBLIC
        sample_gui/include
    )
endif()

