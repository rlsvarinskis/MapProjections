#ifndef IMAGES_H
#define IMAGES_H

#include <string>

#include <GL/glew.h>

struct Image {
    unsigned int width, height;
    unsigned char channels;
    unsigned char *data;
};

struct Texture {
    GLuint texture_id;
    unsigned int width, height;
    float sx, sy;
};

bool load_image(const std::string &name, struct Image &image);
void free_image(struct Image &image);

bool load_texture(const std::string &name, struct Texture &texture, unsigned int x = 0, unsigned int y = 0, int w = 0, int h = 0);
void free_texture(struct Texture &texture);

#endif
