# Map projection experiment

This program allows you to reproject any map onto new coordinates.

Currently, it only supports the equirectangular projection.

## Controls

Click and drag to move the map around.

 - SPACE to reorient north up and south down.
 - X to toggle between locked north mode.
 - 1-9 to change between one of the 9 default maps.
 - ESC to exit.

## Dependencies

This program depends on OpenGL 3.3.

When building, this program depends on libjpeg, libglew, libglfw3, and OpenGL.

## Building

I am still figuring out the build system. 

libjpeg:
./configure --host='x86_64-w64-mingw32' CC='x86_64-w64-mingw32-gcc' --prefix='/usr/x86_64-w64-mingw32'
make
make install

libglew:
make SYSTEM=linux-mingw64 GLEW_DEST=/usr/x86_64-w64-mingw32
make install SYSTEM=linux-mingw64 GLEW_DEST=/usr/x86_64-w64-mingw32