cmake_minimum_required(VERSION 3.10)
project(minesweeper)

find_package(raylib CONFIG REQUIRED)

# file(GLOB SRC_FILES src/*.cpp)
set(SRC_FILES src/main.cpp)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)