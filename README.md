# Map projection experiment

![A different projection of Earth](/screenshots/remapped.png?raw=true "A different projection of Earth")

This program allows you to reproject any map onto new coordinates.

Currently, it only supports the equirectangular projection.

## Controls

Click and drag to move the map around. Scroll in to zoom in. Middle click to rotate around the center.

 - SPACE to reorient north up and south down.
 - X to toggle between locked north mode.
 - 1-9 to change between one of the 9 default maps.
 - ESC to exit.

## Dependencies

This program depends on OpenGL 3.3. It needs glfw3.dll, glew32.dll, and libjpeg-9.dll, and the res/images and res/shaders folders to work.

When building, this program depends on libjpeg, libglew, libglfw3, and OpenGL. Right now, the program is only capable of loading .jpg files.

## Building

I am still figuring out the build system.

libjpeg:
./configure --host='x86_64-w64-mingw32' CC='x86_64-w64-mingw32-gcc' --prefix='/usr/x86_64-w64-mingw32'
make
make install

libglew:
make SYSTEM=linux-mingw64 GLEW_DEST=/usr/x86_64-w64-mingw32
make install SYSTEM=linux-mingw64 GLEW_DEST=/usr/x86_64-w64-mingw32