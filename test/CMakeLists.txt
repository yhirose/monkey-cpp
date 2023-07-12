cmake_minimum_required(VERSION 3.22)
project(test)

add_executable(test-main
  test-code.cpp
  test-compiler.cpp
  test-evaluator.cpp
  test-main.cpp
  test-object.cpp
  test-parser.cpp
  test-symbol_table.cpp
  test-util.hpp
  test-vm.cpp
  test-main.cpp
)

target_include_directories(test-main PRIVATE
  ${peglib_SOURCE_DIR}
  ../engine
)

target_link_libraries(test-main PRIVATE
  fmt::fmt
)
