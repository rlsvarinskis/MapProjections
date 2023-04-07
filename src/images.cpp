#include "images.h"

#include <string>
#include <stdio.h>
#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <jpeglib.h>
#include <jerror.h>
#include <png.h>

static bool load_jpeg(const std::string &filename, struct Image &image) {
    FILE *file = fopen(filename.c_str(), "rb");

    if (!file) {
        std::cerr << "Failed to open " << filename << "!" << std::endl;
        return false;
    }

    unsigned char *rowptr[1];

    //TODO: worry about errors

    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;

    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);

    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);

    jpeg_start_decompress(&info);

    image.width = info.output_width;
    image.height = info.output_height;
    image.channels = info.num_components;

    image.data = new unsigned char[image.width * image.height * image.channels];
    while (info.output_scanline < image.height) {
        rowptr[0] = image.data + info.output_scanline * image.channels * image.width;
        jpeg_read_scanlines(&info, rowptr, 1);
    }
    jpeg_finish_decompress(&info);

    fclose(file);

    return true;
}

static bool load_png(const std::string &filename, struct Image &image) {
    FILE *file = fopen(filename.c_str(), "rb");

    if (!file) {
        std::cerr << "Failed to open " << filename << "!" << std::endl;
        return false;
    }

    unsigned char header[8];
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    unsigned char color_type;
    unsigned char bit_depth;

    fread(header, 1, 8, file);
    if (png_sig_cmp(header, 0, 8)) {
        std::cerr << "Invalid png: " << filename << std::endl;
        goto error_png_header;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        std::cerr << "Failed to create png read struct" << std::endl;
        goto error_png;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        std::cerr << "Failed to set up png info struct" << std::endl;
        goto error_png;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "Failed to prepare reading png file" << std::endl;
        goto error_png;
    }

    png_init_io(png_ptr, file);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    image.width = png_get_image_width(png_ptr, info_ptr);
    image.height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    } else if (bit_depth < 8) {
        png_set_packing(png_ptr);
    }
    image.channels = png_get_channels(png_ptr, info_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
        image.channels = 3;
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0) {
        png_set_tRNS_to_alpha(png_ptr);
        image.channels++;
    }

    int x, y;
    int number_of_passes;
    png_bytep *row_pointers;

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    //png_get_rowbytes(png_ptr, info_ptr);
    image.data = new unsigned char[image.width * image.height * image.channels];
    row_pointers = new png_bytep[image.height];

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "Failed to read image" << std::endl;
        goto error_png_jmp;
    }

    for (int y = 0; y < image.height; y++) {
        row_pointers[y] = &image.data[y * image.width * image.channels];
    }

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(file);
    return true;

error_png_jmp:
    delete[] image.data;
    delete[] row_pointers;
error_png:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
error_png_header:
    fclose(file);
    return false;
}

bool load_image(const std::string &filename, struct Image &image) {
    std::size_t lp = filename.find_last_of('.');
    if (lp == std::string::npos) {
        std::cerr << "Could not determine file extension for " << filename << "!" << std::endl;
        return false;
    }
    std::string ext = filename.substr(lp + 1);

    if (ext.size() > 4) {
        std::cerr << "Unknown image file extension: ." << ext << std::endl;
        return false;
    }

    for (std::size_t i = 0; i < ext.size(); i++) {
        if (ext[i] >= 'A' && ext[i] <= 'Z') {
            ext[i] = ext[i] - 'A' + 'a';
        }
    }

    if (ext == "jpg" || ext == "jpeg") {
        return load_jpeg(filename, image);
    } else if (ext == "png") {
        return load_png(filename, image);
    }

    std::cerr << "Unknown image file extension: ." << ext << std::endl;
    return false;
}

void free_image(struct Image &image) {
    delete[] image.data;
}

static inline bool is_pow2(unsigned int x) {
    return (x & (x - 1)) == 0;
}

static inline unsigned int get_pow2(unsigned int x) {
    if (is_pow2(x)) {
        return x;
    }
    while (!is_pow2(x)) {
        x &= ~(x ^ (x - 1));
    }
    return x * 2;
}

bool load_texture(const std::string &name, struct Texture &texture, unsigned int x, unsigned int y, int w, int h) {
    std::string filename = "./res/images/" + name;

    struct Image image;
    if (!load_image(filename, image)) {
        std::cerr << "Failed to load texture: " << name << std::endl;
        return false;
    }

    // If the texture needs to be cropped, or its size is not a power of 2
    if (x != 0 || y != 0 || (w != 0 && w != image.width) || (h != 0 && h != image.height) || !is_pow2(image.width) || !is_pow2(image.height)) {
        if (w <= 0) {
            w = image.width + w - x;
        }
        if (h <= 0) {
            h = image.height + h - y;
        }
        unsigned int nw = get_pow2(w);
        unsigned int nh = get_pow2(h);
        unsigned char* cropped_data = new unsigned char[nw * nh * image.channels];
        for (int i = 0; i < nw * nh * image.channels; i++) {
            cropped_data[i] = 0xFF;
        }

        unsigned int original_i = (y * image.width + x) * image.channels;
        unsigned int original_skip = (image.width - w) * image.channels;
        unsigned int destination_i = 0;
        unsigned int destination_skip = (nw - w) * image.channels;

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w * image.channels; j++) {
                cropped_data[destination_i++] = image.data[original_i++];
            }
            original_i += original_skip;
            destination_i += destination_skip;
        }
        texture.sx = (float) w / (float) nw;
        texture.sy = (float) h / (float) nh;

        delete[] image.data;
        image.data = cropped_data;
        image.width = nw;
        image.height = nh;
    } else {
        texture.sx = 1;
        texture.sy = 1;
    }

    texture.width = image.width;
    texture.height = image.height;

    GLuint type;
    if (image.channels == 1) {
        type = GL_RED;
    } else if (image.channels == 2) {
        type = GL_RG;
    } else if (image.channels == 3) {
        type = GL_RGB;
    } else if (image.channels == 4) {
        type = GL_RGBA;
    } else {
        std::cerr << "Unknown number of channels in " << name << ": " << image.channels << std::endl;
        free_image(image);
        return false;
    }

    glGenTextures(1, &texture.texture_id);
    glBindTexture(GL_TEXTURE_2D, texture.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, type, texture.width, texture.height, 0, type, GL_UNSIGNED_BYTE, image.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    free_image(image);

    return true;
}
