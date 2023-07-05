# Template from
# https://support.gurobi.com/hc/en-us/articles/360039499751-How-do-I-use-CMake-to-build-Gurobi-C-C-projects-
cmake_minimum_required(VERSION 3.24)
project(
  amod
  VERSION 1.0
  DESCRIPTION "AMOD Project"
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Visual Studio compiler with static runtime libraries
if(MSVC AND MT)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
endif()

find_package(GUROBI REQUIRED)

include_directories(${GUROBI_INCLUDE_DIRS})

add_executable(
  amod
  # other files go here
  src/main.c)

target_link_libraries(${CMAKE_PROJECT_NAME} ${GUROBI_LIBRARY})

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  include(FeatureSummary)
  feature_summary(WHAT ALL)
endif()
