# SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

pkg_check_modules(LIBHEIF IMPORTED_TARGET libheif)
if(NOT LIBHEIF_FOUND)
    message(STATUS "libheif not found: no HEIF/HEIC/AVIF image plugin")
    return()
endif()

add_library(qheif MODULE
    heifhandler.cpp
    heifhandler.h
    heifplugin.cpp
    heifplugin.h
)
target_compile_definitions(qheif PRIVATE QT_NO_KEYWORDS)
set_target_properties(qheif PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/imageformats")
target_link_libraries(qheif Qt6::Core Qt6::Gui PkgConfig::LIBHEIF)
install(TARGETS qheif DESTINATION ${CMAKE_INSTALL_QTPLUGINDIR}/imageformats/)
