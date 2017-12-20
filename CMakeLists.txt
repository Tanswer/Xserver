cmake_minimum_required (VERSION 2.6)
project (Xserver)

file(GLOB SOURCES "src/*.c")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -O2 -Wall  -pthread ")
add_executable(Xserver ${SOURCES})
add_subdirectory(test)
