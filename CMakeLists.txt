cmake_minimum_required(VERSION 3.17)

project(avio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")
if (WIN32)
    add_compile_options("/EHsc")
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

include(GNUInstallDirs)
# set arch-specific include dir
set(CMAKE_INSTALL_INCLUDEDIR "include/${CMAKE_LIBRARY_ARCHITECTURE}")


if (FFMPEG_INCLUDE_DIRS AND FFMPEG_LIBRARIES)
    message("-- FFmpeg is using include dirs ${FFMPEG_INCLUDE_DIRS} and libs ${FFMPEG_LIBRARIES}")
else()
    find_package(FFmpeg)
    if (FFMPEG_INCLUDE_DIRS)
        message("-- Found FFmpeg Include Directories ${FFMPEG_INCLUDE_DIRS}")
    else()    
        message("-- **WARNING** Unable to find FFmpeg Include Directories automatically.  Use -DFFMPEG_INCLUDE_DIRS location manually")
    endif()
    if (FFMPEG_LIBRARIES)
        message("-- Found FFmpeg Libraries ${FFMPEG_LIBRARIES}")
    else()    
        message("-- **WARNING** Unable to find FFmpeg Libraries automatically.  Use -DFFMPEG_LIBRARIES to set location manually")
    endif()
endif()

if (UNIX)
    message("finding alsa")
    set (ALSA_INCLUDE_DIR_CANDIDATE $ENV{CONDA_PREFIX}/include/alsa)
    if (EXISTS "${ALSA_INCLUDE_DIR_CANDIDATE}")
        set (ALSA_INCLUDE_DIR ${ALSA_INCLUDE_DIR_CANDIDATE})
        message("-- Found ALSA include directory ${ALSA_INCLUDE_DIR}")
    endif()
    set (ALSA_LIBRARIES_CANDIDATE $ENV{CONDA_PREFIX}/lib/libasound.so)
    if (EXISTS "${ALSA_LIBRARIES_CANDIDATE}")
        set (ALSA_LIBRARIES ${ALSA_LIBRARIES_CANDIDATE})
        message("-- Found ALSA libraries ${ALSA_LIBRARIES}")
    endif()
endif()

if (SDL2_LIBRARIES AND SDL2_INCLUDE_DIRS)
    message("-- SDL2 is using include dirs ${SDL2_INCLUDE_DIRS} and lib ${SDL2_LIBRARIES}")
else()
    find_package(SDL2)
    if (SDL2_FOUND)
        set(SDL2_LIBRARIES SDL2::SDL2)
        message("-- Found SDL2: ${SDL2_INCLUDE_DIRS}")
    else()
        message("-- **WARNING** Unable to find SDL2 automatically.  Use -DSDL2_INCLUDE_DIRS and -DSDL2_LIBRARIES to set locations manually")
    endif()
endif()


add_library(avio SHARED 
                        src/Reader.cpp
                        src/Exception.cpp
                        src/Event.cpp
                        src/Frame.cpp
                        src/Decoder.cpp
                        src/Filter.cpp
                        src/Clock.cpp
                        src/Display.cpp
                        src/Encoder.cpp
                        src/Writer.cpp
                        src/Pipe.cpp
                    )
                
target_link_libraries(avio PRIVATE 
                ${FFMPEG_LIBRARIES}
                ${SDL2_LIBRARIES}
             )

target_include_directories(avio PUBLIC
                    include
                    ${FFMPEG_INCLUDE_DIRS}
                    ${SDL2_INCLUDE_DIRS}
                  )

if (MSVC)
    install(TARGETS avio
                    LIBRARY DESTINATION lib
                    ARCHIVE DESTINATION lib
                    RUNTIME DESTINATION bin
                    COMPONENT library)
else()
    install(TARGETS avio DESTINATION "${CMAKE_INSTALL_LIBDIR}/")
endif()
install(FILES 
            include/avio.h 
            include/Clock.h
            include/Decoder.h
            include/Display.h
            include/Encoder.h
            include/Event.h
            include/Exception.h
            include/Filter.h
            include/Frame.h
            include/Pipe.h
            include/Queue.h
            include/Reader.h
            include/Writer.h
            
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
