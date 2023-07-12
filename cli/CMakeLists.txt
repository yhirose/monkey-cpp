cmake_minimum_required(VERSION 3.22)
project(cli)

add_executable(monkey
  main.cpp
)

target_include_directories(monkey PRIVATE
  ${peglib_SOURCE_DIR}
  ../engine
)

target_link_libraries(monkey PRIVATE
  fmt::fmt
)
