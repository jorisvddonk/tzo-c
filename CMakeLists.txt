cmake_minimum_required(VERSION 3.14)

if(ADDITIONAL_CMAKE_TOOLCHAIN)
  include(${ADDITIONAL_CMAKE_TOOLCHAIN})
endif(ADDITIONAL_CMAKE_TOOLCHAIN)


include(FetchContent)

project(Tzo VERSION 1.0
                  DESCRIPTION "Tzo runtime VM"
                  LANGUAGES C)

set(ADDITIONAL_LIBRARIES "")
if (UNIX)
list(APPEND ADDITIONAL_LIBRARIES "m")
endif (UNIX)
if(MINGW)
list(APPEND ADDITIONAL_LIBRARIES "gdi32" "user32" "winmm")
endif(MINGW)

add_library(tzo STATIC tzo.c)
add_library(questvm STATIC questvm.c)

add_executable(main main.c)
target_link_libraries(main PUBLIC tzo ${ADDITIONAL_LIBRARIES})

add_executable(test test.c)
target_link_libraries(test PUBLIC tzo ${ADDITIONAL_LIBRARIES})

if (BUILD_CONVO)
  FetchContent_Declare(
    doslike
    GIT_REPOSITORY https://github.com/mattiasgustavsson/dos-like.git
    GIT_TAG 39cda441f25b08ead3262ad6fdb5567270b260fe
  )
  FetchContent_Populate(doslike)
  include_directories(AFTER ${doslike_SOURCE_DIR}/source)

  add_executable(questmark questmark.c doslike_additions.c)
  target_link_libraries(questmark PUBLIC tzo ${ADDITIONAL_LIBRARIES})

  find_package(CURL CONFIG)
  FetchContent_Declare(
    lodepng
    GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
    GIT_TAG 8c6a9e30576f07bf470ad6f09458a2dcd7a6a84a
  )
  FetchContent_Populate(lodepng)
  include_directories(AFTER ${lodepng_SOURCE_DIR})
  set_source_files_properties(${lodepng_SOURCE_DIR}/lodepng.cpp PROPERTIES LANGUAGE C)

  add_executable(convo convo.c questvm.c doslike_additions.c ${lodepng_SOURCE_DIR}/lodepng.cpp)
  target_link_libraries(convo PUBLIC tzo ${ADDITIONAL_LIBRARIES} PRIVATE CURL::libcurl physfs-static)
endif(BUILD_CONVO)

if (BUILD_RCONVO)
  find_package(raylib 4.0 QUIET)
  if (NOT raylib_FOUND)
    include(FetchContent)

    FetchContent_Declare(
      raylib
      URL https://github.com/raysan5/raylib/archive/refs/tags/4.0.0.tar.gz
    )

    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED)
      set(FETCHCONTENT_QUIET NO)
      FetchContent_Populate(raylib)

      set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
      add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})

    endif()

  endif()

  FetchContent_Declare(
    physfs
    GIT_REPOSITORY https://github.com/icculus/physfs.git
    GIT_TAG a0dfe220ffb97eef2c78fef2b8971995f221b18d
  )
  FetchContent_MakeAvailable(physfs)
  include_directories(AFTER ${physfs_SOURCE_DIR}/src)

  add_executable(rconvo rconvo.c questvm.c)
  target_link_libraries(rconvo PUBLIC tzo ${ADDITIONAL_LIBRARIES} raylib PRIVATE physfs-static)
  # raylib:
  # Checks if OSX and links appropriate frameworks (Only required on MacOS)
  if (APPLE)
      target_link_libraries(rconvo PRIVATE "-framework IOKit")
      target_link_libraries(rconvo PRIVATE "-framework Cocoa")
      target_link_libraries(rconvo PRIVATE "-framework OpenGL")
  endif()
endif(BUILD_RCONVO)
