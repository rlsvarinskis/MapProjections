#include "images.h"

#include <string>
#include <stdio.h>
#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <jpeglib.h>
#include <jerror.h>

bool load_texture(const std::string &name, struct Texture &texture) {
    std::string filename = "./res/images/" + name;

    FILE *file = fopen(filename.c_str(), "rb");

    if (!file) {
        std::cerr << "Failed to open " << name << "!" << std::endl;
        return false;
    }

    unsigned char *rowptr[1];
    unsigned char *jdata;

    //TODO: worry about errors

    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;

    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);

    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);

    jpeg_start_decompress(&info);

    texture.width = info.output_width;
    texture.height = info.output_height;

    GLuint type;
    if (info.num_components == 1) {
        type = GL_RED;
    } else if (info.num_components == 2) {
        type = GL_RG;
    } else if (info.num_components == 3) {
        type = GL_RGB;
    } else if (info.num_components == 4) {
        type = GL_RGBA;
    } else {
        std::cerr << "Unknown number of channels in " << name << ": " << info.num_components << std::endl;
        return false;
    }

    jdata = new unsigned char[texture.width * texture.height * 3];
    while (info.output_scanline < texture.height) {
        rowptr[0] = jdata + info.output_scanline * 3 * texture.width;
        jpeg_read_scanlines(&info, rowptr, 1);
    }
    jpeg_finish_decompress(&info);

    fclose(file);

    glGenTextures(1, &texture.texture_id);
    glBindTexture(GL_TEXTURE_2D, texture.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, type, texture.width, texture.height, 0, type, GL_UNSIGNED_BYTE, jdata);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    delete[] jdata;

    return true;
}