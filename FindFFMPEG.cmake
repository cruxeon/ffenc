#.rst:
# FindFFMPEG
# ----------
#
# Find the native FFMPEG includes and library
#
# This module defines::
#
#  FFMPEG_INCLUDE_DIR, where to find avcodec.h, avformat.h ...
#  FFMPEG_LIBRARIES, the libraries to link against to use FFMPEG.
#  FFMPEG_FOUND, If false, do not try to use FFMPEG.
#
# also defined, but not for general use are::
#
#   FFMPEG_avformat_LIBRARY, where to find the FFMPEG avformat library.
#   FFMPEG_avcodec_LIBRARY, where to find the FFMPEG avcodec library.
#
# This is useful to do it this way so that we can always add more libraries
# if needed to ``FFMPEG_LIBRARIES`` if ffmpeg ever changes...

#=============================================================================
# Copyright: 1993-2008 Ken Martin, Will Schroeder, Bill Lorensen
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of YCM, substitute the full
#  License text for the above reference.)

# Originally from VTK project

if(WIN32)
    set(FFMPEG_INCLUDE_DIR $ENV{FFMPEG_DIR}/include)
    find_library(FFMPEG_avformat_LIBRARY avformat
      $ENV{FFMPEG_DIR}
      $ENV{FFMPEG_DIR}/lib
      $ENV{FFMPEG_DIR}/libavformat
    )

    find_library(FFMPEG_avcodec_LIBRARY avcodec
      $ENV{FFMPEG_DIR}
      $ENV{FFMPEG_DIR}/lib
      $ENV{FFMPEG_DIR}/libavcodec
    )

    find_library(FFMPEG_avutil_LIBRARY avutil
      $ENV{FFMPEG_DIR}
      $ENV{FFMPEG_DIR}/lib
      $ENV{FFMPEG_DIR}/libavutil
    )

    if(NOT DISABLE_SWSCALE)
      find_library(FFMPEG_swscale_LIBRARY swscale
        $ENV{FFMPEG_DIR}
        $ENV{FFMPEG_DIR}/lib
        $ENV{FFMPEG_DIR}/libswscale
      )
    endif(NOT DISABLE_SWSCALE)

    find_library(FFMPEG_avdevice_LIBRARY avdevice
      $ENV{FFMPEG_DIR}
      $ENV{FFMPEG_DIR}/lib
      $ENV{FFMPEG_DIR}/libavdevice
    )

    find_library(_FFMPEG_z_LIBRARY_ z
      $ENV{FFMPEG_DIR}
      $ENV{FFMPEG_DIR}/lib
    )
else()
    set(FFMPEG_INCLUDE_DIR /usr/include/ffmpeg)
    find_library(FFMPEG_avformat_LIBRARY avformat
    PATHS
      /usr/local/lib
      /usr/lib
      /usr/lib64
    NO_DEFAULT_PATH
    )

    find_library(FFMPEG_avcodec_LIBRARY avcodec
      /usr/local/lib
      /usr/lib
      /usr/lib64
    NO_DEFAULT_PATH
    )

    find_library(FFMPEG_avutil_LIBRARY avutil
      /usr/local/lib
      /usr/lib
      /usr/lib64
    NO_DEFAULT_PATH
    )

    if(NOT DISABLE_SWSCALE)
      find_library(FFMPEG_swscale_LIBRARY swscale
        /usr/local/lib
        /usr/lib
       /usr/lib64
    NO_DEFAULT_PATH
      )
    endif(NOT DISABLE_SWSCALE)

    find_library(FFMPEG_avdevice_LIBRARY avdevice
      /usr/local/lib
      /usr/lib
      /usr/lib64
    NO_DEFAULT_PATH
    )

    find_library(_FFMPEG_z_LIBRARY_ z
      /usr/local/lib
      /usr/lib
      /usr/lib64
    NO_DEFAULT_PATH
    )
endif()

if(FFMPEG_INCLUDE_DIR)
  if(FFMPEG_avformat_LIBRARY)
    if(FFMPEG_avcodec_LIBRARY)
      if(FFMPEG_avutil_LIBRARY)
        set(FFMPEG_FOUND "YES")
        set(FFMPEG_LIBRARIES ${FFMPEG_avformat_LIBRARY}
                             ${FFMPEG_avcodec_LIBRARY}
                             ${FFMPEG_avutil_LIBRARY}
          )
        if(FFMPEG_swscale_LIBRARY)
          set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES}
                               ${FFMPEG_swscale_LIBRARY}
          )
        endif()
        if(FFMPEG_avdevice_LIBRARY)
          set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES}
                               ${FFMPEG_avdevice_LIBRARY}
          )
        endif()
        if(_FFMPEG_z_LIBRARY_)
          set( FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES}
                                ${_FFMPEG_z_LIBRARY_}
          )
        endif()
      endif()
    endif()
  endif()
endif()

mark_as_advanced(
  FFMPEG_INCLUDE_DIR
  FFMPEG_avformat_LIBRARY
  FFMPEG_avcodec_LIBRARY
  FFMPEG_avutil_LIBRARY
  FFMPEG_swscale_LIBRARY
  FFMPEG_avdevice_LIBRARY
  _FFMPEG_z_LIBRARY_
  )

# Set package properties if FeatureSummary was included
if(COMMAND set_package_properties)
  set_package_properties(FFMPEG PROPERTIES DESCRIPTION "A complete, cross-platform solution to record, convert and stream audio and video")
  set_package_properties(FFMPEG PROPERTIES URL "http://ffmpeg.org/")
endif()
