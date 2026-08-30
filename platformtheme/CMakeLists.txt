project(platformthemeplugin)
include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets QuickControls2 DBus)
find_package(Qt6Gui CONFIG QUIET COMPONENTS Private)
if(NOT TARGET Qt6::GuiPrivate)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()
find_package(KF6WindowSystem REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Qt6XdgIconLoader REQUIRED IMPORTED_TARGET Qt6XdgIconLoader)
find_path(QT6XDGICONLOADER_PRIVATE_INCLUDE_DIR
    NAMES private/xdgiconloader/xdgiconloader_p.h
    PATHS ${Qt6XdgIconLoader_INCLUDE_DIRS} /usr/include/qt6xdgiconloader
    PATH_SUFFIXES ${Qt6XdgIconLoader_VERSION}
    REQUIRED
)

# xcb-ewmh was never used by this plugin and is no longer pulled in by Qt.
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb x11)

set (SRCS
    main.cpp
    platformtheme.h
    platformtheme.cpp
    hintsettings.h
    hintsettings.cpp
    systemtrayicon.h
    systemtrayicon.cpp
    x11integration.h
    x11integration.cpp
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

add_library(cutefishplatformtheme MODULE ${SRCS})

target_compile_definitions(cutefishplatformtheme
    PRIVATE
        "QT_NO_FOREACH"
        "LIB_FM_QT_SONAME=\"${LIB_FM_QT_SONAME}\""
)

target_link_libraries(cutefishplatformtheme PRIVATE
    Qt6::GuiPrivate
    Qt6::Widgets
    Qt6::QuickControls2
    Qt6::Core
    Qt6::DBus
    PkgConfig::Qt6XdgIconLoader

    KF6::WindowSystem
    PkgConfig::XCB
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
