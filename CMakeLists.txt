module_version(1.0)

# On Windows, download the ffmpeg binary packages.
# on OSX and linux, assume the ffmpeg dev libraries are installed.
if(WIN32)
    set(EXTLIB_DIR ${CMAKE_BINARY_DIR}/3rdparty/ffmpeg)
    if(OMEGA_ARCH_32)
        set(FFMPEG_FILENAME ffmpeg-20160213-git-588e2e3-win32)
    else()
        set(FFMPEG_FILENAME ffmpeg-20160213-git-588e2e3-win64)
    endif()
    
    if(NOT EXISTS ${EXTLIB_DIR})
        # Download & extract ffmpeg libs
        message("Downloading ffmpeg development libraries...")
        if(OMEGA_ARCH_32)
            file(DOWNLOAD 
                http://ffmpeg.zeranoe.com/builds/win32/shared/${FFMPEG_FILENAME}-shared.7z
                ${EXTLIB_DIR}/ffmpeg-shared.7z SHOW_PROGRESS)
            file(DOWNLOAD 
                http://ffmpeg.zeranoe.com/builds/win32/dev/${FFMPEG_FILENAME}-dev.7z
                ${EXTLIB_DIR}/ffmpeg-dev.7z SHOW_PROGRESS)
        else()
            file(DOWNLOAD 
                http://ffmpeg.zeranoe.com/builds/win64/shared/${FFMPEG_FILENAME}-shared.7z
                ${EXTLIB_DIR}/ffmpeg-shared.7z SHOW_PROGRESS)
            file(DOWNLOAD 
                http://ffmpeg.zeranoe.com/builds/win64/dev/${FFMPEG_FILENAME}-dev.7z
                ${EXTLIB_DIR}/ffmpeg-dev.7z SHOW_PROGRESS)
        endif()
        message("Extracting ffmpeg development libraries...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf 
            ${EXTLIB_DIR}/ffmpeg-shared.7z 
            WORKING_DIRECTORY ${EXTLIB_DIR})
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf 
            ${EXTLIB_DIR}/ffmpeg-dev.7z 
            WORKING_DIRECTORY ${EXTLIB_DIR})
            
        # Copy dlls in bin folders
        foreach(f IN ITEMS 
                avcodec-57 avdevice-57 avfilter-6 avformat-57 avutil-55 
                postproc-54 swresample-2 swscale-4)
            file(COPY ${EXTLIB_DIR}/${FFMPEG_FILENAME}-shared/bin/${f}.dll DESTINATION ${CMAKE_BINARY_DIR}/bin/debug)
            file(COPY ${EXTLIB_DIR}/${FFMPEG_FILENAME}-shared/bin/${f}.dll DESTINATION ${CMAKE_BINARY_DIR}/bin/release)
        endforeach()
    endif()
    set(ENV{FFMPEG_DIR} ${EXTLIB_DIR}/${FFMPEG_FILENAME}-dev)
    
    SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
find_package(FFMPEG REQUIRED)
if(NOT FFMPEG_FOUND)
    message(ERROR "FFMPEG not found. ffenc module cannot be built.")
    return()
endif()

include_directories(${FFMPEG_INCLUDE_DIR})
add_library(ffenc SHARED ffenc.cpp)
set_target_properties(ffenc PROPERTIES FOLDER modules)
target_link_libraries(ffenc omegaToolkit ${FFMPEG_LIBRARIES})
#declare_native_module(ffenc)


if(true)
#if(OMEGA_BUILD_EXAMPLES)
    add_executable(helloEncoderFFENC
        examples/cube.cpp
        examples/helloEncoderFFENC.cpp)
    set_target_properties(helloEncoderFFENC PROPERTIES FOLDER examples)
    target_link_libraries(helloEncoderFFENC omega ffenc)
endif()