cmake_minimum_required(VERSION 3.0)
project(fakeirc LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror")

add_subdirectory(src)
