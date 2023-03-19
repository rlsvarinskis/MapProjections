#include <string>

#include <GL/glew.h>

struct Texture {
    GLuint texture_id;
    unsigned int width, height;
};

bool load_texture(const std::string &name, struct Texture &texture);