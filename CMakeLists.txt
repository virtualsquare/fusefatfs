cmake_minimum_required(VERSION 3.12)
project(fusefatfs
    VERSION 0.0.1
    DESCRIPTION "fise/vuse module for fat (12/16/32/exfat)"
    HOMEPAGE_URL "https://github.com/virtualsquare/fusefatfs"
    LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# check availability of headers and libs
set(HEADERS_REQUIRED fuse.h)
set(LIBS_REQUIRED vdeplug_mod ssl crypto)

set(CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64)

foreach(HEADER IN LISTS HEADERS_REQUIRED)
  check_include_file(${HEADER} ${HEADER}_OK)
  if(NOT ${HEADER}_OK)
    message(FATAL_ERROR "header file ${HEADER} not found")
  endif()
endforeach(HEADER)

foreach(THISLIB IN LISTS LIBS_REQUIRED)
  find_library(LIB${THISLIB}_OK ${THISLIB})
  if(NOT LIB${THISLIB}_OK)
    message(FATAL_ERROR "library lib${THISLIB} not found")
  endif()
endforeach(THISLIB)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2 -O2 -pedantic -Wall -Wextra")

add_definitions(-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -DFFF_ATOMIC)
# add_definitions(-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64)

add_executable(fusefatfs fusefatfs.c fftable.c diskio.c ff.c ffunicode.c)
target_link_libraries(fusefatfs fuse)
install(TARGETS fusefatfs
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_library(vufusefatfs SHARED fusefatfs.c fftable.c diskio.c ff.c ffunicode.c)
set_target_properties(vufusefatfs PROPERTIES PREFIX "")
install(TARGETS vufusefatfs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/vu/modules)

add_subdirectory(man)

add_custom_target(uninstall
  "${CMAKE_COMMAND}"
    -DCADO_SPOOL_DIR=${CADO_SPOOL_DIR}
    -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake"
  )

