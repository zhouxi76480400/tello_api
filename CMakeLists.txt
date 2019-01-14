cmake_minimum_required(VERSION 3.12)
project(tello_api)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(tello_api main.cpp tello_api/tello.h tello_api/tello.cpp)