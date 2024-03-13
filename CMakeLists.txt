cmake_minimum_required(VERSION 3.27)
project(fivedxp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS -m32)

include_directories(Include)

add_library(fivedxp SHARED library.cpp
        jvs.cpp
        touch.cpp
        bana.cpp
        config.cpp
        memory.cpp
        input.cpp
)

target_link_libraries(fivedxp ${CMAKE_SOURCE_DIR}/Libs/libdobby.a SDL2)