cmake_minimum_required(VERSION 3.26)
project(gauss)

find_package(glm)

add_executable(main main.cpp)

target_link_libraries(main glm)