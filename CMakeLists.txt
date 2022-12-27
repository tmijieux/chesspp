cmake_minimum_required(VERSION 3.10)

project(chesspp CXX)

# find_package(Boost COMPONENTS program_options iostreams regex REQUIRED)
list(APPEND
  CMAKE_MODULE_PATH
  "${PROJECT_SOURCE_DIR}/cmake/"
  "${PROJECT_SOURCE_DIR}/SDL2-2.26.1/cmake/"
  "${PROJECT_SOURCE_DIR}/SDL2_image-2.6.2/cmake/"
)

include("sdl2-config")
include("sdl2_image-config")

add_subdirectory(fmt EXCLUDE_FROM_ALL)
add_subdirectory(src)
