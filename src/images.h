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
};

bool load_image(const std::string &name, struct Image &image);
void free_image(struct Image &image);

bool load_texture(const std::string &name, struct Texture &texture);
void free_texture(struct Texture &texture);
