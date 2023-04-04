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

/**
 * This code keeps 3 different textures, because y = 2x + sin(2x) has a steep slope at first
 * (i.e. there are a lot of y values for a small change in x, therefore a lot of samples of x
 * in the sampling texture), but the slope quickly goes to 0 at the very end, so y does not
 * change much for a large change in x. This code therefore keeps much more samples toward
 * the end.
 * 
 * This probably overcomplicates the code without too much benefit, but hopefully it makes
 * the projection more precise.
 */
static const int cnt_000 = /*2048*/512;
static const int cnt_300 = 4096;
static const int cnt_314 = 8192;
static const int cnt = cnt_000 - 1 + cnt_300 - 1 + cnt_314;

/**
 * This stores the inverse of Kepler's equation, except the index doesn't have a linear
 * relationship with the value of y.
 */
static float values[cnt];

static GLuint texture_000_300;
static GLuint texture_300_314;
static GLuint texture_314_pi;

/**
 * Calculate the index of a y value.
 * This function is necessary, because it isn't a linear relationship, because
 * the 3 regions of the function have a different amount of samples.
 */
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
    double mid = (l + r) / 2;
    double res = 2 * mid + std::sin(2 * mid);
    int m = val_to_index(res);

    values[m] = mid;

    if (m > li + 1) {
        generate_values_between(li, m, l, mid);
    }
    if (m < ri - 1) {
        generate_values_between(m, ri, mid, r);
    }
}

/**
 * The following code precomputes the inverse of y = 2x + sin(2x) using a binary search
 * algorithm. Rather than attempt to estimate the inverse, it starts with (y = 0 -> x = 0)
 * and (y = pi -> x = pi/2) and then recursively calculates y for the x in the middle of
 * the range.
 */
static bool generate_values_cpu() {
    generate_values_between(0, cnt - 1, 0, PI / 2);
    values[0] = 0;
    values[cnt - 1] = PI / 2;

    return true;
}

/**
 * TODO: experiment with using a compute shader to generate values
 */
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

/**
 * Mollweide's projection makes use of the equation y = 2x + sin(2x), but to calculate
 * the y coordinate of a certain latitude requires solving for x given some y. There is
 * no closed-form solution for solving this
 * (https://en.wikipedia.org/wiki/Kepler%27s_equation#Inverse_problem), so a method for
 * estimating it is required.
 * 
 * It then stores the computed values in a texture that the shader uses to lookup the
 * inverse given any y.
 */
bool mollweide_prepare_input_shader(const unsigned int image_width, const unsigned int image_height, const GLuint shader_program) {
    if (!textures_prepared) {
        if (!generate_values_cpu()) {
            std::cerr << "Failed to generate values!" << std::endl;
            return false;
        }

        if (!prepare_texture()) {
            std::cerr << "Failed to prepare texture!" << std::endl;
            return false;
        }

        textures_prepared = true;
    }

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
    GLint t000_o_id = glGetUniformLocation(shader_program, "t000_offset");
    GLint t300_o_id = glGetUniformLocation(shader_program, "t300_offset");
    GLint t314_o_id = glGetUniformLocation(shader_program, "t314_offset");

    if (t000_id < 0 || t300_id < 0 || t314_id < 0 || t000_o_id < 0 || t300_o_id < 0 || t314_o_id < 0) {
        return false;
    }

    glUniform1i(t000_id, 4);
    glUniform1i(t300_id, 5);
    glUniform1i(t314_id, 6);

    glUniform1f(t000_o_id, 0.5f / (float) cnt_000);
    glUniform1f(t300_o_id, 0.5f / (float) cnt_300);
    glUniform1f(t314_o_id, 0.5f / (float) cnt_314);

    return true;
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
