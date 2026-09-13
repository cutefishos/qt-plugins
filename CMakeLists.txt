cmake_minimum_required(VERSION 3.16)

# C is needed for the generated Wayland protocol marshalling.
project(cutefish-qt-plugins LANGUAGES C CXX)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(platformtheme)
add_subdirectory(imageformat)
