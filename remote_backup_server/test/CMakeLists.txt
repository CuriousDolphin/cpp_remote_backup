cmake_minimum_required(VERSION 3.15)
project(test)

set(CMAKE_CXX_STANDARD 17)
include_directories(asio/include)
include_directories(asio/include/asio)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASIO_STANDALONE -pthread")

add_executable(test main.cpp asio)