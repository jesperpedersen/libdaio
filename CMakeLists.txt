# 
# The MIT License (MIT)
# 
# Copyright (c) 2017 Jesper Pedersen <jesper.pedersen@redhat.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the Software
# is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
cmake_minimum_required (VERSION 2.8)
set(PROJECT_NAME daio)
project (${PROJECT_NAME})

set(CMAKE_C_FLAGS "-g -O2 -Wall")

find_path(LIBAIO_INCLUDE_DIR libaio.h HINTS PATH_SUFFIXES include)
find_library(LIBAIO_LIBRARY aio HINTS PATH_SUFFIXES lib)
mark_as_advanced(LIBAIO_INCLUDE_DIR LIBAIO_LIBRARY)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libaio DEFAULT_MSG LIBAIO_INCLUDE_DIR LIBAIO_LIBRARY)

if (LIBAIO_FOUND)
  set(LIBAIO_LIBRARIES "${LIBAIO_LIBRARY}")
  include_directories(${LIBAIO_INCLUDE_DIR})
  link_libraries(${LIBAIO_LIBRARIES})
else (LIBAIO_FOUND)
  message(FATAL_ERROR "libaio not found")
endif(LIBAIO_FOUND)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

add_library(daio SHARED libdaio.c)
add_executable(libdaio_example libdaio_example.c)

target_link_libraries(libdaio_example LINK_PUBLIC daio)
