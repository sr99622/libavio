#*******************************************************************************
# libavio/samples/CMakeLists.txt
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

project(libavio_samples VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")

if(WIN32)
    add_compile_options("/EHsc")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(skeletal
    skeletal/main.cpp
)

target_link_libraries(skeletal
    libavio
)

add_executable(basic
    basic/main.cpp
)

target_link_libraries(basic PRIVATE
    libavio
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(gui
    gui/src/glwidget.cpp
    gui/src/main.cpp
    gui/src/mainwindow.cpp
    gui/src/progress.cpp
    gui/include/glwidget.h
    gui/include/mainwindow.h
    gui/include/progress.h
)

target_link_libraries(gui PRIVATE
    Qt5::Widgets
    libavio
)

target_include_directories(gui PUBLIC
    gui/include
)
