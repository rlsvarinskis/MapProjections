#include "mollweide.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <cmath>
#include <iostream>

// Magic constant
const float PI = 3.141592653589793238462;

bool mollweide_xy_to_uv(const double x, const double y, double &u, double &v) {
    v = std::asin(y);
    u = x * PI / std::cos(v);
    v = std::asin((2 * v + std::sin(2 * v)) / PI);
    if (u < -PI || u > PI) {
        return false;
    }
    return !std::isnan(u) && !std::isnan(v);
}

static bool textures_prepared = false;

static const int cnt_000 = 2048;
static const int cnt_300 = 4096;
static const int cnt_314 = 8192;
static const int cnt = cnt_000 - 1 + cnt_300 - 1 + cnt_314;

static float values[cnt];

static GLuint texture_000_300;
static GLuint texture_300_314;
static GLuint texture_314_pi;

static inline int val_to_index(double v) {
    if (v <= 3.0) {
        return (int) ((v / 3.0) * (cnt_000 - 1));
    }
    if (v <= 3.14) {
        return cnt_000 - 1 + (int) ((v - 3.0) / 0.14 * (cnt_300 - 1));
    }
    return cnt_000 - 1 + cnt_300 - 1 + (int) ((v - 3.14) / (PI - 3.14) * (cnt_314 - 1));
}

static void generate_values_between(int li, int ri, double l, double r) {
    //std::cout << "Generating values between " << li << " and " << ri << " (" << l << "; " << r << ")" << std::endl;
    double mid = (l + r) / 2;
    double res = 2 * mid + std::sin(2 * mid);
    int m = val_to_index(res);
    //std::cout << "Got mid " << mid << " -> " << res << " (index " << m << ")" << std::endl;

    values[m] = mid;

    if (m > li + 1) {
        generate_values_between(li, m, l, mid);
    }
    if (m < ri - 1) {
        generate_values_between(m, ri, mid, r);
    }
}

static bool generate_values_cpu() {
    values[0] = 0;
    values[cnt - 1] = PI / 2;
    generate_values_between(0, cnt - 1, 0, PI / 2);

    return true;
}

static bool generate_values_gpu() {
    return false;
}

static bool prepare_texture() {
    GLuint textures[3];
    int widths[3] = {cnt_000, cnt_300, cnt_314};
    float *data[3] = {values, values + (cnt_000 - 1), values + (cnt_000 - 1 + cnt_300 - 1)};

    glGenTextures(3, textures);

    for (int i = 0; i < sizeof(textures) / sizeof(*textures); i++) {
        glBindTexture(GL_TEXTURE_1D, textures[i]);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, widths[i], 0, GL_RED, GL_FLOAT, data[i]);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    }

    texture_000_300 = textures[0];
    texture_300_314 = textures[1];
    texture_314_pi = textures[2];

    return true;
}

bool mollweide_prepare_input_shader(const unsigned int image_width, const unsigned int image_height, const GLuint shader_program) {
    if (textures_prepared) {
        goto apply_shader;
    }

    if (!generate_values_cpu()) {
        std::cerr << "Failed to generate values!" << std::endl;
        return false;
    }

    if (!prepare_texture()) {
        std::cerr << "Failed to prepare texture!" << std::endl;
        return false;
    }

    textures_prepared = true;

apply_shader:
    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_1D, texture_000_300);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_1D, texture_300_314);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_1D, texture_314_pi);

    GLint t000_id = glGetUniformLocation(shader_program, "texture_000_300");
    GLint t300_id = glGetUniformLocation(shader_program, "texture_300_314");
    GLint t314_id = glGetUniformLocation(shader_program, "texture_314_pi");

    if (t000_id < 0 || t300_id < 0 || t314_id < 0) {
        return false;
    }

    glUniform1i(t000_id, 4);
    glUniform1i(t300_id, 5);
    glUniform1i(t314_id, 6);
    return textures_prepared;
}

// A useful paper explaining the Mollweide projection:
// http://master.grad.hr/hdgg/kog_stranica/kog15/2Lapaine-KoG15.pdf
Projection mollweide = {
    .width = 2,
    .height = 1,
    .shader = "mollweide",
    .xy_to_uv = &mollweide_xy_to_uv,
    .prepare_input = mollweide_prepare_input_shader,
    .prepare_output = nullptr,
    .free_input = nullptr,
    .free_output = nullptr
};