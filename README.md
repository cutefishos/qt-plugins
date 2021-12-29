# Qt Plugins

Unify Qt application style of CutefishOS.

### Archlinux
## Dependencies
`sudo pacman -S gcc extra-cmake-modules qt5-base qt5-tools qt5-x11extras libqtxdg libdbusmenu-qt5 libxcb`

### Debian
## Dependencies
`sudo apt install cmake extra-cmake-modules libqt5x11extras5-dev libkf5windowsystem-dev libqt5xdg-dev libdbusmenu-qt5-dev libxcb-shape0-dev libxcb-icccm4-dev libxcb1-dev libxcb-ewmh-dev qtbase5-dev qtbase5-private-dev qtdeclarative5-dev qtquickcontrols2-5-dev qttools5-dev qttools5-dev-tools`

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
