cmake_minimum_required(VERSION 2.6)
project(viztrace)

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

ADD_EXECUTABLE(silicehe
  src/main.cpp
  src/VCDParser.cpp
  src/VCDParser.h
  src/video_out.cpp
  src/video_out.h
  src/VgaChip.cpp
  src/VgaChip.h
)

TARGET_LINK_LIBRARIES(silicehe LibSL LibSL_gl fstapi Threads::Threads)

INSTALL(TARGETS silicehe RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/../../bin)
