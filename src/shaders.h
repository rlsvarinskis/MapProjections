#ifndef SHADERS_H
#define SHADERS_H

#include <string>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

struct Shader {
    GLuint program_id;
    GLuint vertex_id;
    GLuint fragment_id;
};

bool read_shader(const std::string shadername, std::string &result);
bool load_shader(const std::string &shadername, const std::string &vertex_shader_code, const std::string &fragment_shader_code, Shader &shader);
bool load_shader(const std::string &shadername, Shader &shader);
void free_shader(Shader &shader);

#endif
