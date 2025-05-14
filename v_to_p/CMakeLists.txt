cmake_minimum_required(VERSION 3.11)
project(v_to_p)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

Set(CMAKE_CXX_STANDARD 20)
Set(CMAKE_CXX_STANDARD_REQUIRED True)
Set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(
    include 
)

add_executable(
    magic 
    src/main.cpp
    src/functions.c
    src/utils.c
    src/slice-timing-oracle.cpp
    src/stats.c
    src/translation-oracle.cpp
    src/core-config.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-pointer-arith")
