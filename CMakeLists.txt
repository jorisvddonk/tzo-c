cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(Tzo VERSION 1.0
                  DESCRIPTION "Tzo runtime VM"
                  LANGUAGES C)

set(ADDITIONAL_LIBRARIES "")
if(MINGW)
list(APPEND ADDITIONAL_LIBRARIES "gdi32" "user32" "winmm")
endif(MINGW)

add_library(tzo STATIC tzo.c)

add_executable(main main.c)
target_link_libraries(main PUBLIC tzo ${ADDITIONAL_LIBRARIES})

add_executable(test test.c)
target_link_libraries(test PUBLIC tzo ${ADDITIONAL_LIBRARIES})

FetchContent_Declare(
  doslike
  GIT_REPOSITORY https://github.com/mattiasgustavsson/dos-like.git
  GIT_TAG 39cda441f25b08ead3262ad6fdb5567270b260fe
)
FetchContent_Populate(doslike)
include_directories(AFTER ${doslike_SOURCE_DIR}/source)

# needs dos-like:
add_executable(questmark questmark.c ${doslike_SOURCE_DIR}/source/dos.c)
target_link_libraries(questmark PUBLIC tzo ${ADDITIONAL_LIBRARIES})


#FetchContent_Declare(
#  lodepng
#  GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
#  GIT_TAG 8c6a9e30576f07bf470ad6f09458a2dcd7a6a84a
#)
#FetchContent_Populate(lodepng)
#include_directories(AFTER ${lodepng_SOURCE_DIR})

# needs modded dos-like, libcurl, lode-png and physfs
#add_executable(convo convo.c ${doslike_SOURCE_DIR}/source/dos.c ${lodepng_SOURCE_DIR}/lodepng.cpp)
#target_link_libraries(convo PUBLIC tzo)