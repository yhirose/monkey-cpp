cmake_minimum_required(VERSION 3.22)
project(cli)

add_executable(monkey
  ../vendor/fmt/src/format.cc
  main.cpp
)

target_include_directories(monkey PRIVATE
  ../vendor/fmt/include
  ../vendor/cpp-peglib
  ../engine
)
