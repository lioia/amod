cmake_minimum_required(VERSION 3.24)
project(
  amod_tests
  VERSION 1.0
  DESCRIPTION "AMOD Project - Tests"
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_executable(amod_tests runner.c)

target_link_libraries(amod_tests amod_library)

add_test(NAME amod_tests COMMAND amod_tests)
