cmake_minimum_required(VERSION 3.24)
project(
  amod_tests
  VERSION 1.0
  DESCRIPTION "AMOD Project - Tests"
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} runner.c)
add_test(NAME ${PROJECT_NAME} COMMAND ${CMAKE_PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARY_NAME})
