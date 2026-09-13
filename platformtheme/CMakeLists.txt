project(platformthemeplugin)
include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus)
find_package(Qt6Gui CONFIG QUIET COMPONENTS Private)
if(NOT TARGET Qt6::GuiPrivate)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(Qt6XdgIconLoader REQUIRED IMPORTED_TARGET Qt6XdgIconLoader)
find_path(QT6XDGICONLOADER_PRIVATE_INCLUDE_DIR
    NAMES private/xdgiconloader/xdgiconloader_p.h
    PATHS ${Qt6XdgIconLoader_INCLUDE_DIRS} /usr/include/qt6xdgiconloader
    PATH_SUFFIXES ${Qt6XdgIconLoader_VERSION}
    REQUIRED
)

set (SRCS
    main.cpp
    waylandexport.h
    waylandexport.cpp
    platformtheme.h
    platformtheme.cpp
    filedialoghelper.h
    filedialoghelper.cpp
    hintsettings.h
    hintsettings.cpp
    systemtrayicon.h
    systemtrayicon.cpp
    statusnotifier/dbustypes.h
    statusnotifier/dbustypes.cpp
    statusnotifier/statusnotifieritem.h
    statusnotifier/statusnotifieritem.cpp
)

qt6_add_dbus_interface(SRCS org.kde.StatusNotifierWatcher.xml statusnotifierwatcher_interface)

qt6_add_dbus_adaptor(SRCS
    statusnotifier/org.kde.StatusNotifierItem.xml
    statusnotifier/statusnotifieritem.h
    StatusNotifierItem
)

# xdg-foreign lets the portal's file dialog parent itself onto the application
# window. Optional: without it dialogs simply open unparented.
pkg_check_modules(WaylandClient QUIET IMPORTED_TARGET wayland-client)
pkg_check_modules(WaylandProtocols QUIET wayland-protocols>=1.32)
if(WaylandClient_FOUND AND WaylandProtocols_FOUND)
    pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
    pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
    set(THEME_PROTOCOL_DIR ${CMAKE_CURRENT_BINARY_DIR}/wayland-protocols)
    file(MAKE_DIRECTORY ${THEME_PROTOCOL_DIR})
    set(xdg_foreign_xml
        ${WAYLAND_PROTOCOLS_DATADIR}/unstable/xdg-foreign/xdg-foreign-unstable-v2.xml)
    add_custom_command(OUTPUT ${THEME_PROTOCOL_DIR}/xdg-foreign-unstable-v2-protocol.c
        COMMAND ${WAYLAND_SCANNER} private-code ${xdg_foreign_xml}
            ${THEME_PROTOCOL_DIR}/xdg-foreign-unstable-v2-protocol.c
        DEPENDS ${xdg_foreign_xml} VERBATIM)
    add_custom_command(OUTPUT ${THEME_PROTOCOL_DIR}/xdg-foreign-unstable-v2-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} client-header ${xdg_foreign_xml}
            ${THEME_PROTOCOL_DIR}/xdg-foreign-unstable-v2-client-protocol.h
        DEPENDS ${xdg_foreign_xml} VERBATIM)
    list(APPEND SRCS
        ${THEME_PROTOCOL_DIR}/xdg-foreign-unstable-v2-protocol.c
        ${THEME_PROTOCOL_DIR}/xdg-foreign-unstable-v2-client-protocol.h)
endif()

add_library(cutefishplatformtheme MODULE ${SRCS})

if(WaylandClient_FOUND AND WaylandProtocols_FOUND)
    target_compile_definitions(cutefishplatformtheme PRIVATE CUTEFISH_PLATFORMTHEME_WAYLAND)
    target_include_directories(cutefishplatformtheme PRIVATE ${THEME_PROTOCOL_DIR})
    target_link_libraries(cutefishplatformtheme PRIVATE PkgConfig::WaylandClient)
endif()

target_compile_definitions(cutefishplatformtheme
    PRIVATE
        "QT_NO_FOREACH"
        "LIB_FM_QT_SONAME=\"${LIB_FM_QT_SONAME}\""
)

target_link_libraries(cutefishplatformtheme PRIVATE
    Qt6::GuiPrivate
    Qt6::Widgets
    Qt6::Core
    Qt6::DBus
    PkgConfig::Qt6XdgIconLoader

)
target_include_directories(cutefishplatformtheme PRIVATE "${QT6XDGICONLOADER_PRIVATE_INCLUDE_DIR}")

find_program(QT_PATHS_EXECUTABLE NAMES qtpaths6 REQUIRED)
execute_process(COMMAND "${QT_PATHS_EXECUTABLE}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE QT_PLUGINS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(QT_PLUGINS_DIR)
    message(STATUS "Qt6 plugin directory:" "${QT_PLUGINS_DIR}")
else()
    message(FATAL_ERROR "Qt6 plugin directory cannot be detected.")
endif()

install(TARGETS cutefishplatformtheme LIBRARY DESTINATION "${QT_PLUGINS_DIR}/platformthemes")
