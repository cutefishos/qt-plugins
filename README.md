# Qt Plugins

Qt plugins that make Qt applications at home in the Cutefish session. The
`cutefish` platform theme makes them follow the system's dark mode, fonts,
icon and cursor themes, open the Cutefish file chooser, and scroll with the
same touchpad feel as Cutefish's own apps. The image format plugins add SVG,
HEIF/HEIC/AVIF and JPEG XL.

## Dependencies

`sudo pacman -S gcc cmake qt6-base qt6-declarative qt6-tools wayland wayland-protocols librsvg libheif libjxl`

## Build

```shell
mkdir build
cd build
cmake ..
make
sudo make install
```

## License

BSD-3-Clause; see [LICENSE](LICENSE).
