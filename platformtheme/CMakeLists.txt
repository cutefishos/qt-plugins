project(platformthemeplugin)
include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5QuickControls2 REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5X11Extras REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED Private)
find_package(Qt5XdgIconLoader REQUIRED)
find_package(dbusmenu-qt5 REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(PkgConfig REQUIRED)

# dependencies for QPA plugin
find_package(Qt5ThemeSupport REQUIRED)
set(QT5PLATFORMSUPPORT_LIBS Qt5ThemeSupport::Qt5ThemeSupport)

# qdbusmenubar uses them
remove_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)

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
    qdbusmenubar_p.h
    qdbusmenubar.cpp
    x11integration.h
    x11integration.cpp
    statusnotifier/dbustypes.h
    statusnotifier/dbustypes.cpp
    statusnotifier/statusnotifieritem.h
    statusnotifier/statusnotifieritem.cpp
)

qt5_add_dbus_interface(SRCS org.kde.StatusNotifierWatcher.xml statusnotifierwatcher_interface)

qt5_add_dbus_adaptor(SRCS
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
    Qt5::GuiPrivate
    Qt5::X11Extras
    Qt5::Widgets
    Qt5::QuickControls2
    Qt5::Core
    Qt5::DBus
    dbusmenu-qt5

    Qt5XdgIconLoader

    KF5::WindowSystem
    PkgConfig::XCB
    ${QT5PLATFORMSUPPORT_LIBS}
)

find_program(QT_QMAKE_EXECUTABLE NAMES qmake-qt5 qmake REQUIRED)
execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE QT_PLUGINS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(QT_PLUGINS_DIR)
    message(STATUS "Qt5 plugin directory:" "${QT_PLUGINS_DIR}")
else()
    message(FATAL_ERROR "Qt5 plugin directory cannot be detected.")
endif()

install(TARGETS cutefishplatformtheme LIBRARY DESTINATION "${QT_PLUGINS_DIR}/platformthemes")
