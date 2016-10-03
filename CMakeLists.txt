PROJECT(indi-ffmpeg C CXX)
cmake_minimum_required(VERSION 2.4.7)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

set(CMAKE_CXX_FLAGS "-g -std=c++0x ${CMAKE_CXX_FLAGS}")
#Include (CheckCXXSourceCompiles)

#SET(CMAKE_CXX_FLAGS "-Wall" )
#SET(CMAKE_CXX_FLAGS_DEBUG "-Werror -O0 -g -DDEBUG_FITS" )
#SET(CMAKE_C_FLAGS "-Wall" )
#SET(CMAKE_C_FLAGS_DEBUG "-Werror -O0 -g -DDEBUG_FITS" )

include (MacroOptionalFindPackage)
include (MacroLogFeature)
include (MacroBoolTo01)


find_package(INDI REQUIRED)
find_package(CFITSIO REQUIRED)
#find_package(Nova REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(FFmpeg REQUIRED)

set(FFMPEG_VERSION_MAJOR 0)
set(FFMPEG_VERSION_MINOR 1)
#option(WITH_ALIGN_GEEHALEL "Enable Alignment Test" ON)
#option(WITH_SIMULATOR "Enable Simulator" ON)
#option(WITH_NOFMANY "Use NOFMANY switches" ON)
#option(STOP_WHEN_MOTION_CHANGED "Stop motors when motion mode changes" ON)
#option(WITH_SCOPE_LIMITS "Enable Scope limits" ON)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
set(INDI_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/indi")

if (NOT CFITSIO_FOUND OR CFITSIO_VERSION_MAJOR LESS 3)
  message(FATAL_ERROR "CFITSIO version too old, Please install cfitsio 3.x and try again. http://heasarc.gsfc.nasa.gov/fitsio/fitsio.html")
endif (NOT CFITSIO_FOUND OR CFITSIO_VERSION_MAJOR LESS 3)

macro_bool_to_01(CFITSIO_FOUND HAVE_CFITSIO_H)
macro_log_feature(CFITSIO_FOUND "libcfitsio" "A library for reading and writing data files in FITS (Flexible Image Transport System) data format" "http://heasarc.gsfc.nasa.gov/fitsio/fitsio.html" FALSE "3.03" "Provides INDI with FITS I/O support.")

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${FFMPEG_INCLUDE_DIR})
#include_directories( ${NOVA_INCLUDE_DIR})
#include_directories( ${NOVA_INCLUDE_DIR}/..)
if (CFITSIO_FOUND)
  include_directories(${CFITSIO_INCLUDE_DIR})
endif (CFITSIO_FOUND)

########### OpenCV ###############
set(ffmpeg_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg.cpp )
#if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#  set(eqmod_SRCS ${eqmod_SRCS}
#    ${CMAKE_CURRENT_SOURCE_DIR}/mach_gettime.cpp)
#endif()

#if(WITH_SIMULATOR)
#  set(eqmod_SRCS ${eqmod_SRCS}
#   ${CMAKE_CURRENT_SOURCE_DIR}/simulator/simulator.cpp    ${CMAKE_CURRENT_SOURCE_DIR}/simulator/skywatcher-simulator.cpp)
#endif(WITH_SIMULATOR)

add_executable(indi_ffmpeg_ccd ${ffmpeg_SRCS})

target_link_libraries(indi_ffmpeg_ccd  ${INDI_LIBRARIES} ${INDI_DRIVER_LIBRARIES} ${FFMPEG_LIBRARIES} -lavdevice -lswscale ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

#if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
# for itimers
target_link_libraries(indi_ffmpeg_ccd rt)
#endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

install(TARGETS indi_ffmpeg_ccd RUNTIME DESTINATION bin )

install( FILES  indi_ffmpeg.xml indi_ffmpeg_sk.xml DESTINATION ${INDI_DATA_DIR})

#if (WITH_SIMULATOR)
#  install( FILES  simulator/indi_eqmod_simulator_sk.xml DESTINATION ${INDI_DATA_DIR})
#endif(WITH_SIMULATOR)

