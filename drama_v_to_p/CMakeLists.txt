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
    drama
    src/main.cpp
    src/functions.c
    src/utils.c
    src/slice-timing-oracle.cpp
    src/stats.c
    src/translation-oracle.cpp
    src/core-config.cpp
)

# unprivileged
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-pointer-arith")
# unprivileged but debug prints
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-pointer-arith -DDEBUG")
# priveleged, dont measure slice via timing, just use the slice function
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-pointer-arith -DDEBUG_FAST_MODE -DDEBUG")
# priveleged, check consecutive but dont use oracle at all (just directly translate virt to phys)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-pointer-arith -DDEBUG_SUPER_FAST_MODE -DDEBUG_FAST_MODE -DDEBUG")
# priveleged, dont use framework at all, just directly translate virt to phys, dont even check if consecutive
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-pointer-arith -DDEBUG_ULTRA_FAST_MODE -DDEBUG")
