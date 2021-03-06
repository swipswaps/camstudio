# Copyright (C) 2018  Steven Hoving
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

project(CamEncoder)

find_package(ffmpeg)

set(ENCODER_SOURCE
    src/av_audio.cpp
    src/av_dict.cpp
    src/av_error.cpp
    src/av_muxer.cpp
    src/av_video.cpp
    src/av_log.h
)

set(ENCODER_INCLUDE
    include/CamEncoder/av_audio.h
    include/CamEncoder/av_config.h
    include/CamEncoder/av_dict.h
    include/CamEncoder/av_error.h
    include/CamEncoder/av_muxer.h
    include/CamEncoder/av_icodec.h
    include/CamEncoder/av_video.h
    include/CamEncoder/av_ffmpeg.h
    include/CamEncoder/av_encoder.h
)

set(ENCODER_CAM_ENCODER_SOURCE
    src/av_cam_codec/av_cam_codec.cpp
)

set(ENCODER_CAM_ENCODER_INCLUDE
    include/CamEncoder/av_cam_codec/av_cam_codec.h
)

source_group(src FILES
    ${ENCODER_SOURCE}
    ${ENCODER_INCLUDE}
)

source_group(src\\cam_codec FILES
    ${ENCODER_CAM_ENCODER_SOURCE}
    ${ENCODER_CAM_ENCODER_INCLUDE}
)

add_library(CamEncoder STATIC
    ${ENCODER_SOURCE}
    ${ENCODER_INCLUDE}
    ${ENCODER_CAM_ENCODER_SOURCE}
    ${ENCODER_CAM_ENCODER_INCLUDE}
    ${ENCODER_YUV_SOURCE}
    ${ENCODER_YUV_INCLUDE}
)

target_include_directories(CamEncoder
  PUBLIC
    include
    ${FFMPEG_INCLUDE_DIR}
)

target_compile_definitions(CamEncoder
  PRIVATE
    NOMINMAX
    _UNICODE
    UNICODE
)

target_compile_options(CamEncoder
  PRIVATE
    /experimental:external
    /external:W0
    /external:anglebrackets
)

target_link_libraries(CamEncoder
  PUBLIC
    fmt
    libminilzo
    zlibstatic
    yuvconvert
    ${FFMPEG_LIBRARIES}
)

install(
  FILES
    "${FFMPEG_BIN_DIR}/avcodec-58.dll"
    "${FFMPEG_BIN_DIR}/avdevice-58.dll"
    "${FFMPEG_BIN_DIR}/avfilter-7.dll"
    "${FFMPEG_BIN_DIR}/avformat-58.dll"
    "${FFMPEG_BIN_DIR}/avutil-56.dll"
    "${FFMPEG_BIN_DIR}/postproc-55.dll"
    "${FFMPEG_BIN_DIR}/swresample-3.dll"
    "${FFMPEG_BIN_DIR}/swscale-5.dll"
  DESTINATION bin
)

add_subdirectory(tests)
