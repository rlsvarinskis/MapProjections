# Map projection experiment

![A different projection of Earth](/screenshots/earth_robinson.png?raw=true "A different projection of Earth")

![A different projection of Mars](/screenshots/mars_mollweide.png?raw=true "A different projection of Mars")

This program allows you to reproject any map projection onto new coordinates.

### TODO:

- Cleanup of OpenGL objects and better error handling
- Render during resize
- Better rotation behavior (the principle should be to minimize visible rotation/distortion around the mouse)
- WebGL version

## Controls

Click and drag to move the map around. Scroll in to zoom in. Middle click to rotate around the center.

 - `ASDFG` to select between using the equirectangular, Mollweide, Hammer, Azimuthal equidistant, or Robinson projections.
 - `1-9` to change between one of the 9 default maps.
 - `QWERTY` to select between images of Earth, the Moon, Mars, Jupiter, Saturn, or the heatmap of the universe.
 - `SPACE` to reorient north up and south down.
 - `X` to toggle between locked north mode.
 - `ESC` to exit.

## Dependencies

This program depends on OpenGL 3.3.

Make sure the executable has access to the `res/images` and `res/shaders` folders. Right now, the program is only capable of loading .jpg and .png files from the res/images folder.

Linux requires `libjpeg8`, `libglew2.2`, and `libglfw3` to run. To install on Ubuntu, run the following commands:

```
sudo apt update && sudo apt install libjpeg8 libglew2.2 libglfw3
```

## Building

### Windows using MSYS2

Make sure the following packages have been installed for the environment you are targetting (for example `mingw-w64-x86_64`): `toolchain`, `cmake`, `libjpeg`, `glew`, `glfw`, `ninja`.

Run `cmake -G Ninja -B .build -DCMAKE_BUILD_TYPE=Release` in the source folder to configure the builder, then run `cmake --build .build` to build everything. The following files will be placed in the `.build` folder: `MapProjection.exe` and `res`.

### Ubuntu

When building, this program depends on `libjpeg-dev`, `libglew-dev`, `libglfw3-dev`, and `libopengl-dev`. Install with the following commands:

```
sudo apt update && sudo apt install libjpeg-dev libopengl-dev libglew-dev libglfw3-dev
```

Run `cmake -G Ninja -B .build -DCMAKE_BUILD_TYPE=Release -DOpenGL_GL_PREFERENCE=GLVND` in the source folder to configure the builder, then run `cmake --build .build` to build everything. The following files will be placed in the `.build` folder: `MapProjection` and `res`.

### Others

You will have to figure this out yourself. It should just be a matter of getting `libjpeg`, `libglew`, `libglfw3`, and `libopengl` on your system together with their header files, then compiling with `cmake`.
