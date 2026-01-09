#/********************************************************************
# libavio/cmake/FindFFmpeg.cmake
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
            $ENV{FFMPEG_INSTALL_DIR}/lib
            $ENV{CONDA_PREFIX}
    )
endmacro()

macro(find_component_include_dir var header_name)
    find_path(${var} ${header_name}
        HINTS
            $ENV{FFMPEG_INSTALL_DIR}/include
            $ENV{CONDA_PREFIX}
    )
endmacro()

if (DEFINED ENV{FFMPEG_INSTALL_DIR})
    find_component_library(LIBAVCODEC_LIBRARY avcodec)
    find_component_include_dir(LIBAVCODEC_INCLUDE_DIR libavcodec/avcodec.h)

    find_component_library(LIBAVDEVICE_LIBRARY avdevice)
    find_component_include_dir(LIBAVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
    
    find_component_library(LIBAVFILTER_LIBRARY avfilter)
    find_component_include_dir(LIBAVFILTER_INCLUDE_DIR libavfilter/avfilter.h)
    
    find_component_library(LIBAVFORMAT_LIBRARY avformat)
    find_component_include_dir(LIBAVFORMAT_INCLUDE_DIR libavformat/avformat.h)
    
    find_component_library(LIBAVUTIL_LIBRARY avutil)
    find_component_include_dir(LIBAVUTIL_INCLUDE_DIR libavutil/avutil.h)
    
    find_component_library(LIBSWRESAMPLE_LIBRARY swresample)
    find_component_include_dir(LIBSWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
    
    find_component_library(LIBSWSCALE_LIBRARY swscale)
    find_component_include_dir(LIBSWSCALE_INCLUDE_DIR libswscale/swscale.h)

    set(FFMPEG_LINK_LIBRARIES
        ${LIBAVCODEC_LIBRARY}
        ${LIBAVDEVICE_LIBRARY}
        ${LIBAVFILTER_LIBRARY}
        ${LIBAVFORMAT_LIBRARY}
        ${LIBAVUTIL_LIBRARY}
        ${LIBSWRESAMPLE_LIBRARY}
        ${LIBSWSCALE_LIBRARY}
    )

    set(FFMPEG_INCLUDE_DIRS
        ${LIBAVCODEC_INCLUDE_DIR}
        ${LIBAVDEVICE_INCLUDE_DIR}
        ${LIBAVFILTER_INCLUDE_DIR}
        ${LIBAVFORMAT_INCLUDE_DIR}
        ${LIBAVUTIL_INCLUDE_DIR}
        ${LIBSWRESAMPLE_INCLUDE_DIR}
        ${LIBSWSCALE_INCLUDE_DIR}
    )

    include(FindPackageHandleStandardArgs)

    FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFmpeg
        REQUIRED_VARS FFMPEG_INCLUDE_DIRS FFMPEG_LINK_LIBRARIES
        VERSION_VAR FFMPEG_VERSION_STRING
    )
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
            libavcodec 
            libavfilter 
            libavformat 
            libavutil 
            libswscale 
            libswresample
        )
    endif()
endif()

if (FFMPEG_FOUND)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    set_target_properties(FFmpeg::FFmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${FFMPEG_LINK_LIBRARIES}")
endif()
