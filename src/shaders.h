#include <string>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

struct Shader {
    GLuint program_id;
    GLuint vertex_id;
    GLuint fragment_id;
};

bool load_shader(const std::string &shadername, Shader &shader);
