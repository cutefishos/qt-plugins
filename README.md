# Qt Plugins

Unify Qt application style of CutefishOS.

`waylandegl` is the `cutefish-egl` client buffer integration: Qt's wayland-egl with the
viewport destination committed together with each buffer, so Qt Quick windows do not
stretch while resizing. cutefish-compositor points the host's Qt 6 clients at it.
It is a port of Qt 6.8's integration and is only built against Qt 6.8.

## Dependencies

`sudo pacman -S gcc qt6-base qt6-tools libqtxdg librsvg`

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
