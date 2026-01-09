#/********************************************************************
# libavio/cmake-win/FindSDL2.cmake
#
# Copyright (c) 2022  Stephen Rhodes
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
#*********************************************************************/

macro(find_component_library var library_name)
    find_library(${var} NAME ${library_name}
        HINTS
            $ENV{SDL2_INSTALL_DIR}/lib
            $ENV{CONDA_PREFIX}
    )
endmacro()

macro(find_component_include_dir var header_name)
    find_path(${var} ${header_name}
        HINTS
            $ENV{SDL2_INSTALL_DIR}/include
            $ENV{CONDA_PREFIX}
    )
endmacro()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(SDL2 
        sdl2
    )
else()
    find_component_library(SDL2_LIBRARY SDL2)
    find_component_include_dir(SDL2_INCLUDE_DIR SDL/SDL.h)

    set(SDL2_LINK_LIBRARIES
        ${SDL2_LIBRARY}
    )

    set(SDL2_INCLUDE_DIRS
        ${SDL2_INCLUDE_DIR}/SDL
    )

    include(FindPackageHandleStandardArgs)

    FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2
        REQUIRED_VARS SDL2_INCLUDE_DIRS SDL2_LINK_LIBRARIES
        VERSION_VAR SDL2_VERSION_STRING
    )
endif()

if (SDL2_FOUND)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${SDL2_LINK_LIBRARIES}")
endif()
