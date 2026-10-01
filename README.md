# Qt Plugins

Qt plugins that fit Qt applications into the Cutefish session.

`platformtheme` is the `cutefish` platform theme: fonts, colour scheme, palette and
icon theme from the appearance settings, file dialogs through the file chooser
portal, and tray icons as StatusNotifierItems.

`imageformat` reads SVG images through librsvg.

`waylandegl` is the `cutefish-egl` client buffer integration: Qt's wayland-egl with the
viewport destination committed together with each buffer, so Qt Quick windows do not
stretch while resizing. cutefish-compositor points the host's Qt 6 clients at it.
It is a port of Qt 6.8's integration and is only built against Qt 6.8.

## Dependencies

`sudo pacman -S gcc cmake qt6-base qt6-tools qt6-wayland wayland wayland-protocols librsvg`

## Build

```shell
mkdir build
cd build
cmake ..
make
sudo make install
```

## License

cutefish-qt-plugins is licensed under GPLv3.
