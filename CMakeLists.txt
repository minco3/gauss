cmake_minimum_required(VERSION 3.26)
project(gauss)

set(CMAKE_CXX_STANDARD 23)

find_package(glm)
find_package(SDL2)

add_subdirectory(imgui)

add_executable(main main.cpp)

target_link_libraries(main SDL2::SDL2)
target_link_libraries(imgui SDL2_Image)

target_link_libraries(main imgui)

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    target_link_libraries(main glm)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    target_link_libraries(main glm::glm)
endif()