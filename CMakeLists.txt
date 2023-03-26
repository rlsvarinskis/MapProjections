cmake_minimum_required(VERSION 3.10)

project(MapProjection)

if (WIN32)
    set(GLEW_USE_STATIC_LIBS)
    set(ZLIB_USE_STATIC_LIBS)
endif()

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(MapProjection
    src/main.cpp
    src/shaders.cpp
    src/images.cpp
    src/projection.cpp
    src/mapper.cpp
    src/maps.cpp
)

target_include_directories(MapProjection
    PRIVATE "${ZLIB_INCLUDE_DIRS}"
    PRIVATE "${JPEG_INCLUDE_DIRS}"
    PRIVATE "${PNG_INCLUDE_DIRS}"
    PRIVATE "${OPENGL_INCLUDE_DIR}"
    PRIVATE "${GLEW_INCLUDE_DIRS}"
    PRIVATE "${GLFW_INCLUDE_DIRS}"
)

if (WIN32)
    target_sources(MapProjection PRIVATE src/icon/icon.rc)
    add_compile_definitions(GLEW_STATIC)

    find_library(ZLIB_LIB "libz.a")
    find_library(JPEG_LIB "libjpeg.a")
    find_library(PNG_LIB "libpng.a")
    find_library(GLFW_LIB "libglfw3.a")

    target_link_libraries(MapProjection "${JPEG_LIB}" "${PNG_LIB}" "${ZLIB_LIB}" OpenGL::GL GLEW::glew_s "${GLFW_LIB}")
    target_link_options(MapProjection PRIVATE -mwindows -static-libgcc -static-libstdc++ -static)
else()
    target_link_libraries(MapProjection ZLIB::ZLIB JPEG::JPEG PNG::PNG OpenGL::GL GLEW::glew glfw)
endif()

file(COPY "${CMAKE_SOURCE_DIR}/res" DESTINATION "${CMAKE_BINARY_DIR}")
