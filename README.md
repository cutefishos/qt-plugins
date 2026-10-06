# Qt Plugins

Qt plugins that fit Qt applications into the Cutefish session: the `cutefish`
platform theme, image format plugins (SVG, HEIF/HEIC/AVIF, JPEG XL) and the
`cutefish-egl` Wayland client buffer integration.

## Dependencies

`sudo pacman -S gcc cmake qt6-base qt6-tools qt6-wayland wayland wayland-protocols librsvg libheif libjxl`

and [cutefish-framework](https://github.com/cutefishos/cutefish-framework).

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
