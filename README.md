# Qt Plugins

Qt plugins that fit Qt applications into the Cutefish session: the `cutefish`
platform theme with the session's touchpad scrolling and image format plugins
(SVG, HEIF/HEIC/AVIF, JPEG XL).

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
