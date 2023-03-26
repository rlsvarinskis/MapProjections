#include "shaders.h"

#include <string>

#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

static bool load_file(const std::string shadername, std::string &result) {
    std::string target = "res/shaders/" + shadername + ".glsl";
    std::ifstream file(target);
    if (file.fail()) {
        std::cerr << "Failed to open " << target << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    result = buffer.str();
    return true;
}

static bool compile_shader(const std::string &name, const std::string &code, GLuint shader_id) {
    GLint result = GL_FALSE;
    int log_length;

    const char* shader_code = code.c_str();

    glShaderSource(shader_id, 1, &shader_code, NULL);
    glCompileShader(shader_id);

    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    if (result == GL_FALSE) {
        std::cerr << "Failed to compile shader " << name << "!" << std::endl;
    }
    if (log_length > 0) {
        char *error = new char[log_length + 1];
        glGetShaderInfoLog(shader_id, log_length, NULL, error);
        std::cerr << "Error log for compiling " << name << ":\n" << error << std::endl;
        delete[] error;
    }

    return result == GL_TRUE;
}

bool load_shader(const std::string &shadername, Shader &shader) {
    std::string vertex_shader_code;
    std::string fragment_shader_code;
    if (!load_file(shadername + ".vert", vertex_shader_code) || !load_file(shadername + ".frag", fragment_shader_code)) {
        return false;
    }

    shader.vertex_id = glCreateShader(GL_VERTEX_SHADER);
    shader.fragment_id = glCreateShader(GL_FRAGMENT_SHADER);

    if (!compile_shader(shadername + ".vert", vertex_shader_code, shader.vertex_id) || !compile_shader(shadername + ".frag", fragment_shader_code, shader.fragment_id)) {
        goto err;
    }

    shader.program_id = glCreateProgram();
    glAttachShader(shader.program_id, shader.vertex_id);
    glAttachShader(shader.program_id, shader.fragment_id);
    glLinkProgram(shader.program_id);

    GLint result; result = GL_FALSE;
    int log_length;
    glGetProgramiv(shader.program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(shader.program_id, GL_INFO_LOG_LENGTH, &log_length);

    if (result == GL_FALSE) {
        std::cerr << "Failed to link shader " << shadername << "!" << std::endl;
    }
    if (log_length > 0) {
        char *error = new char[log_length + 1];
        glGetProgramInfoLog(shader.program_id, log_length, NULL, error);
        std::cerr << "Error log for linking " << shadername << ":\n" << error << std::endl;
        delete[] error;
    }
    if (result == GL_FALSE) {
        glDeleteProgram(shader.program_id);
        goto err;
    }

    return true;

err:
    glDeleteShader(shader.vertex_id);
    glDeleteShader(shader.fragment_id);
    return false;
}

void free_shader(Shader &shader) {
    glDeleteProgram(shader.program_id);
    glDeleteShader(shader.vertex_id);
    glDeleteShader(shader.fragment_id);
}
