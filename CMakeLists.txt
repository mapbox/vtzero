#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#-----------------------------------------------------------------------------

project(vtzero LANGUAGES CXX)

set(VTZERO_VERSION_MAJOR 0)
set(VTZERO_VERSION_MINOR 0)
set(VTZERO_VERSION_PATCH 1)

set(VTZERO_VERSION
    "${VTZERO_VERSION_MAJOR}.${VTZERO_VERSION_MINOR}.${VTZERO_VERSION_PATCH}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------

option(WERROR "Add -Werror flag to build (turns warnings into errors)" ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -Wsign-compare -Wconversion -Wshadow")

if (WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

include_directories("${CMAKE_SOURCE_DIR}/include")


#-----------------------------------------------------------------------------
#
#  Find dependency: protozero
#
#-----------------------------------------------------------------------------

find_package(Protozero 1.5.3 REQUIRED)

include_directories(SYSTEM ${PROTOZERO_INCLUDE_DIR})


#-----------------------------------------------------------------------------

enable_testing()

add_subdirectory(doc)

add_subdirectory(examples)

add_subdirectory(test)


#-----------------------------------------------------------------------------
