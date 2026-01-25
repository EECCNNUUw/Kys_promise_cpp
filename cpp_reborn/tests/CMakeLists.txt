cmake_minimum_required(VERSION 3.10)
project(check_talk)

set(CMAKE_CXX_STANDARD 17)

include_directories(../include)
include_directories(../../SDL3-3.4.0/include)

add_executable(check_talk check_talk.cpp ../src/FileLoader.cpp)
