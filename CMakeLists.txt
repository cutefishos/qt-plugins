cmake_minimum_required(VERSION 3.16)

# C is needed for the generated Wayland protocol marshalling.
project(cutefish-qt-plugins LANGUAGES C CXX)

add_subdirectory(platformtheme)
add_subdirectory(imageformat)
add_subdirectory(waylandegl)
