#include "robinson.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <cmath>
#include <iostream>

// Magic constant
const float PI = 3.141592653589793238462;

/**
 * Hard-coded values
 */
static const double X[19] = { 1.0000, 0.9986, 0.9954, 0.9900, 0.9822, 0.9730, 0.9600, 0.9427, 0.9216, 0.8962, 0.8679, 0.8350, 0.7986, 0.7597, 0.7186, 0.6732, 0.6213, 0.5722, 0.5322 };
static const double Y[19] = { 0.0000, 0.0620, 0.1240, 0.1860, 0.2480, 0.3100, 0.3720, 0.4340, 0.4958, 0.5571, 0.6176, 0.6769, 0.7346, 0.7903, 0.8435, 0.8936, 0.9394, 0.9761, 1.0000 };

static bool values_prepared = false;

static const int cnt = 2048;
static float y_to_l[cnt];
static float l_to_y[cnt];
static float l_to_x[cnt];

static GLuint y_to_l_texture;
static GLuint l_to_y_texture;
static GLuint l_to_x_texture;

static inline double sqr(double t) { return t * t; }

static inline double h00(double t) { return (1 + 2 * t) * sqr(1 - t); }
static inline double h10(double t) { return t * sqr(1 - t); }
static inline double h01(double t) { return sqr(t) * (3 - 2 * t); }
static inline double h11(double t) { return sqr(t) * (t - 1); }

static inline double spline(double t, double x1, double tx1, double x2, double tx2) {
    return h00(t) * x1 + h10(t) * tx1 + h01(t) * x2 + h11(t) * tx2;
}

static bool generate_values_cpu() {
    const int vals = sizeof(X) / sizeof(*X);
    double tangent_x[vals];
    double tangent_y[vals];

    tangent_x[vals - 1] = X[vals - 1] - X[vals - 2];
    tangent_y[vals - 1] = Y[vals - 1] - Y[vals - 2];

    double last_x = X[1];
    double last_y = -Y[1];
    for (int i = 0; i < vals - 1; i++) {
        tangent_x[i] = ((X[i] - last_x) + (X[i + 1] - X[i])) / 2;
        tangent_y[i] = ((Y[i] - last_y) + (Y[i + 1] - Y[i])) / 2;
        last_x = X[i];
        last_y = Y[i];
    }

    l_to_y[cnt - 1] = 1;

    for (int i = 0; i < cnt - 1; i++) {
        y_to_l[i] = -1;
    }
    y_to_l[cnt - 1] = 1;
    y_to_l[0] = 0;

    for (int i = 0; i < cnt - 1; i++) {
        int ind = i * (vals - 1) / (cnt - 1);

        double t = ((i * (vals - 1)) % (cnt - 1)) / (double) (cnt - 1);

        l_to_x[i] = spline(t, X[ind], tangent_x[ind], X[ind + 1], tangent_x[ind + 1]);
        l_to_y[i] = spline(t, Y[ind], tangent_y[ind], Y[ind + 1], tangent_y[ind + 1]);

        //std::cout << ind << ":" << t << ": " << l_to_x[i] << "; " << l_to_y[i] << std::endl;

        y_to_l[(int) (l_to_y[i] * (cnt - 1))] = i / (double) (cnt - 1);
    }
    //std::cout << "Calculated spline!" << std::endl;

    //std::cout << "Reverse:" << std::endl;
    for (int i = 1; i < cnt - 1; i++) {
        //std::cout << i << ": " << y_to_l[i] << std::endl;
        if (y_to_l[i] < 0) {
            //std::cout << "Missing number!" << std::endl;
            int l = i - 1;
            int r = i + 1;
            while (r < cnt && y_to_l[r] < 0) {
                r++;
            }

            double ll = y_to_l[l];
            double lr = y_to_l[r];

            y_to_l[i] = (ll + lr) / 2;

            //std::cout << "Missing range inside " << l << " to " << r << " (" << ll << " to " << lr << ")" << std::endl;

            int cycles = 0;
            while (l + 1 < r) {
                double mid_l = (ll + lr) / 2;
                double mid = mid_l * (vals - 1);
                int ind = (int) mid;
                double t = mid - ind;

                double mid_y = spline(t, Y[ind], tangent_y[ind], Y[ind + 1], tangent_y[ind + 1]);

                int m = (int) (mid_y * (cnt - 1));

                //std::cout << "Taking mid " << mid_l << " @ ind " << ind << ", t " << t << ", got result " << mid_y << " @ " << m << std::endl;

                if (m > l && m < r) {
                    y_to_l[m] = mid_l;
                    r = m;
                    lr = mid_l;
                } else if (m == r) {
                    lr = mid_l;
                } else if (m == l) {
                    ll = mid_l;
                } else {
                    std::cerr << "Bad value!!" << std::endl;
                    break;
                }

                if (cycles++ > 20) {
                    break;
                }
            }
        }
    }

    return true;
}

static bool generate_values_gpu() {
    return false;
}

static bool prepare_texture() {
    GLuint textures[3];
    float *data[3] = {y_to_l, l_to_y, l_to_x};

    glGenTextures(3, textures);

    for (int i = 0; i < sizeof(textures) / sizeof(*textures); i++) {
        glBindTexture(GL_TEXTURE_1D, textures[i]);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, cnt, 0, GL_RED, GL_FLOAT, data[i]);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    }

    y_to_l_texture = textures[0];
    l_to_y_texture = textures[1];
    l_to_x_texture = textures[2];

    return true;
}

bool robinson_prepare_input_shader(const unsigned int image_width, const unsigned int image_height, const GLuint shader_program) {
    if (!values_prepared) {
        if (!generate_values_cpu()) {
            std::cerr << "Failed to generate values!" << std::endl;
            return false;
        }

        if (!prepare_texture()) {
            std::cerr << "Failed to prepare texture!" << std::endl;
            return false;
        }

        values_prepared = true;
    }

    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_1D, l_to_y_texture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_1D, l_to_x_texture);

    GLint y_id = glGetUniformLocation(shader_program, "l_to_y");
    GLint x_id = glGetUniformLocation(shader_program, "l_to_x");
    if (y_id < 0 || x_id < 0) {
        return false;
    }

    glUniform1i(y_id, 4);
    glUniform1i(x_id, 5);

    return true;
}

bool robinson_prepare_output_shader(const unsigned int screen_width, const unsigned int screen_height, const GLuint shader_program) {
    if (!values_prepared) {
        if (!generate_values_cpu()) {
            std::cerr << "Failed to generate values!" << std::endl;
            return false;
        }

        if (!prepare_texture()) {
            std::cerr << "Failed to prepare texture!" << std::endl;
            return false;
        }

        values_prepared = true;
    }

    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_1D, y_to_l_texture);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_1D, l_to_x_texture);

    GLint y_id = glGetUniformLocation(shader_program, "y_to_l");
    GLint x_id = glGetUniformLocation(shader_program, "yl_to_x");
    if (y_id < 0 || x_id < 0) {
        return false;
    }

    glUniform1i(y_id, 8);
    glUniform1i(x_id, 9);

    return true;
}

bool robinson_xy_to_uv(const double x, const double y, double &u, double &v) {
    if (y > 1 || y < -1) {
        return false;
    }

    float mul = 1;
    if (y < 0) {
        mul = -1;
    }

    double l = y_to_l[(int) (mul * y * cnt)];
    u = x / l_to_x[(int) (l * cnt)];
    v = mul * l * PI / 2;

    if (u < -1 || u > 1) {
        return false;
    }

    u *= PI;

    return !std::isnan(u) && !std::isnan(v);
}

// Robinson projection, with values taken from https://en.wikipedia.org/wiki/Robinson_projection
// This uses cubic spline interpolation between values
Projection robinson = {
    .width = 10000,
    .height = 2536 * 2,
    .shader = "robinson",
    .xy_to_uv = robinson_xy_to_uv,
    .prepare_input = robinson_prepare_input_shader,
    .prepare_output = robinson_prepare_output_shader,
    .free_input = nullptr,
    .free_output = nullptr
};