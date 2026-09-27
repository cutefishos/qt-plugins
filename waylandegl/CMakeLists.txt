cmake_minimum_required(VERSION 3.16)

project(cutefish-egl)

set(TARGET cutefish-egl)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# A port of Qt 6.8's wayland-egl integration onto its private API. Other Qt
# versions need a port of their own; until then the plugin is left out and Qt's
# own integration is used.
find_package(Qt6 QUIET COMPONENTS WaylandClient)
if(NOT Qt6WaylandClient_VERSION MATCHES "^6\\.8\\.")
    message(STATUS "cutefish-egl: no port for QtWaylandClient '${Qt6WaylandClient_VERSION}', not built")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL WaylandClient)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WaylandEgl REQUIRED IMPORTED_TARGET wayland-egl)
pkg_check_modules(Egl REQUIRED IMPORTED_TARGET egl)

# cutefish-compositor only offers the plugin to clients running exactly these
# versions: their private ABI is what the plugin was compiled against.
configure_file(cutefish-egl.json.in ${CMAKE_CURRENT_BINARY_DIR}/cutefish-egl.json @ONLY)

set(SRCS
    main.cpp
    qwaylandeglclientbufferintegration.cpp
    qwaylandeglclientbufferintegration_p.h
    qwaylandeglinclude_p.h
    qwaylandeglwindow.cpp
    qwaylandeglwindow_p.h
    qwaylandglcontext.cpp
    qwaylandglcontext_p.h
)

add_library(${TARGET} MODULE ${SRCS})
set_target_properties(${TARGET} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/wayland-graphics-integration-client"
)
target_link_libraries(${TARGET}
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::OpenGL
    Qt6::OpenGLPrivate
    Qt6::WaylandClient
    Qt6::WaylandClientPrivate
    PkgConfig::WaylandEgl
    PkgConfig::Egl
)

find_program(QT_PATHS_EXECUTABLE NAMES qtpaths6 REQUIRED)
execute_process(COMMAND "${QT_PATHS_EXECUTABLE}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE CMAKE_INSTALL_QTPLUGINDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

install(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_QTPLUGINDIR}/wayland-graphics-integration-client/)
