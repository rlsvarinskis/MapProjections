cmake_minimum_required(VERSION 3.10)

project(MapProjection)

if (WIN32)
    set(GLEW_USE_STATIC_LIBS)
endif()

find_package(OpenGL REQUIRED)
find_package(JPEG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(MapProjection
    src/main.cpp
    src/shaders.cpp
    src/images.cpp
)

target_include_directories(MapProjection PRIVATE "${OPENGL_INCLUDE_DIR}" "${JPEG_INCLUDE_DIRS}" "${GLEW_INCLUDE_DIRS}" "${GLFW_INCLUDE_DIRS}")

if (WIN32)
    add_compile_definitions(GLEW_STATIC)

    find_library(JPEG_LIB "libjpeg.a")
    find_library(GLFW_LIB "libglfw3.a")
    target_link_libraries(MapProjection "${JPEG_LIB}" OpenGL::GL GLEW::glew_s "${GLFW_LIB}")
    target_link_options(MapProjection PRIVATE -mwindows -static-libgcc -static-libstdc++ -static)
else()
    target_link_libraries(MapProjection JPEG::JPEG OpenGL::GL GLEW::glew glfw)
endif()

file(COPY "${CMAKE_SOURCE_DIR}/res" DESTINATION "${CMAKE_BINARY_DIR}")
