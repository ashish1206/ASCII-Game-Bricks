cmake_minimum_required(VERSION 3.5)

project(bricks)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread -std=c++14")
set (source_dir "${PROJECT_SOURCE_DIR}/src/")

file (GLOB source_files "${source_dir}/*.cpp")

add_executable (bricks ${source_files})
