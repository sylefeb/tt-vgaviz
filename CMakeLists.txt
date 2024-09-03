cmake_minimum_required(VERSION 2.6)
project(tt-vgaviz)

include(FetchContent)

FetchContent_Declare(
  libsl
  GIT_REPOSITORY https://github.com/sylefeb/LibSL.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(libsl)

INCLUDE_DIRECTORIES(
	${LIBSL_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/
  ${PROJECT_SOURCE_DIR}/libs/fstapi/
)

ADD_SUBDIRECTORY(libs/fstapi)

FIND_PACKAGE(Threads REQUIRED)

# --------------------------------------
# Customize the VGA resolution here

add_compile_definitions(VGA_W=640)
add_compile_definitions(VGA_W_FPORCH=16)
add_compile_definitions(VGA_W_PULSE=96)
add_compile_definitions(VGA_W_BPORCH=48)

add_compile_definitions(VGA_H=480)
add_compile_definitions(VGA_H_FPORCH=10)
add_compile_definitions(VGA_H_PULSE=2)
add_compile_definitions(VGA_H_BPORCH=33)

# --------------------------------------

ADD_EXECUTABLE(tt-vgaviz
  src/main.cpp
  src/VCDParser.cpp
  src/VCDParser.h
  src/video_out.cpp
  src/video_out.h
  src/VgaChip.cpp
  src/VgaChip.h
)

TARGET_LINK_LIBRARIES(tt-vgaviz LibSL LibSL_gl fstapi Threads::Threads)
