#include <iostream>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "GL/glext.h"
#include "GL/gl.h"

#include "shaders.h"
#include "images.h"

// Magic constant
const float PI = 3.141592653589793238462;

static float rot_mat[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

static float ns = 0;
static float ew = 0;

static float roll_animation_amount = 0;
static float roll_animation_start = 0;
static float last_roll_animation_time = 1.0f;
static const float roll_animation_duration = 0.4f;
static bool roll_animation_lock_north = false;

static bool drag_active = false;
static bool rotate_active = false;
static double drag_startx = 0;
static double drag_starty = 0;
static double rotate_startangle = 0;
static double zoom = 1;

static bool lock_north = false;

static GLFWcursor *normal;
static GLFWcursor *grab;

unsigned int current_texture = 9;
Texture textures[9];

static float easeOutQuintic(float time) {
    return 1 - std::pow(1 - time, 5);
}

void handle_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) { //Begin drag
            drag_active = true;
            glfwGetCursorPos(window, &drag_startx, &drag_starty);
            glfwSetCursor(window, grab);
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            drag_startx /= w;
            drag_starty /= h;
        } else { //End drag
            drag_active = false;
            glfwSetCursor(window, normal);
        }
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) { //Begin drag
            rotate_active = true;
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            glfwSetCursor(window, grab);
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            x /= w;
            y /= h;
            x -= 0.5f;
            y -= 0.5f;
            rotate_startangle = std::atan2(y, x);
        } else { //End drag
            rotate_active = false;
            glfwSetCursor(window, normal);
        }
    }
}

void handle_scroll(GLFWwindow *window, double xscroll, double yscroll) {
    zoom *= std::exp(yscroll / 5);
    if (zoom < 1) {
        zoom = 1;
    }
}

static void rotate_roll(float roll) {
    float sz = std::sin(roll), cz = std::cos(roll);

    float rotz[9] = {
        cz, sz, 0,
        -sz, cz, 0,
        0, 0, 1
    };
    
    float rot[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rot[i * 3 + j] = 0;
            for (int k = 0; k < 3; k++) {
                rot[i * 3 + j] += rotz[i * 3 + k] * rot_mat[k * 3 + j];
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        rot_mat[i] = rot[i];
    }
}

static void reset_roll() {
    //North vector (0, 1, 0) will go the 2nd column of rot_mat^T
    float roll = std::atan2(-rot_mat[1], rot_mat[4]); //-x / y, so that up is 0 degrees
    roll_animation_amount = roll;
    roll_animation_start = glfwGetTime();
    last_roll_animation_time = 0;
}

static bool roll_animate(float time) {
    if (last_roll_animation_time >= 1) {
        return false;
    }
    float anim_time = (time - roll_animation_start) / roll_animation_duration;
    if (anim_time >= 1) {
        anim_time = 1;
    }

    float distance = easeOutQuintic(anim_time) - easeOutQuintic(last_roll_animation_time);
    rotate_roll(roll_animation_amount * distance);

    last_roll_animation_time = anim_time;

    if (anim_time == 1) {
        if (roll_animation_lock_north) {
            roll_animation_lock_north = false;
            lock_north = true;
            //Calculate the angle that (0, 0, 1) goes to
            ns = -std::asin(rot_mat[7]);
            ew = -std::atan2(rot_mat[6], rot_mat[8]); //x / z
        }
        return false;
    }
    return true;
}

static void rotate_by(float rx, float ry) {
    float sx = std::sin(rx), cx = std::cos(rx);
    float sy = std::sin(ry), cy = std::cos(ry);

    //r(x) * r(y)
    float rotxy[9] = {
        cy, -sx * sy, cx * sy,
        0, cx, sx,
        -sy, -sx * cy, cx * cy
    };
    
    float rot[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rot[i * 3 + j] = 0;
            for (int k = 0; k < 3; k++) {
                rot[i * 3 + j] += rotxy[i * 3 + k] * rot_mat[k * 3 + j];
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        rot_mat[i] = rot[i];
    }
}

void handle_cursor_position(GLFWwindow *window, double xpos, double ypos) {
    if (!drag_active && !rotate_active) {
        return;
    }

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    xpos /= w;
    ypos /= h;

    if (drag_active) {
        if (lock_north) {
            ew += (xpos - drag_startx) * 2 * PI / zoom;
            ns += (ypos - drag_starty) * PI / zoom;
            if (ns > PI / 2) {
                ns = PI / 2;
            } else if (ns < -PI / 2) {
                ns = -PI / 2;
            }
            rot_mat[0] = 1; rot_mat[1] = 0; rot_mat[2] = 0;
            rot_mat[3] = 0; rot_mat[4] = 1; rot_mat[5] = 0;
            rot_mat[6] = 0; rot_mat[7] = 0; rot_mat[8] = 1;
            rotate_by(0, ew);
            rotate_by(ns, 0);
        } else if (!roll_animation_lock_north) {
            float ox = (drag_starty * 2 - 1.0f) * PI / zoom;
            float oy = (drag_startx * 2 - 1.0f) * PI / zoom;

            rotate_by(0, -oy);

            double dx = (xpos - drag_startx) / zoom;
            double dy = (ypos - drag_starty) / zoom;

            rotate_by(dy * PI, dx * 2 * PI);

            rotate_by(0, oy);
        }

        drag_startx = xpos;
        drag_starty = ypos;
    }
    if (rotate_active && !lock_north && !roll_animation_lock_north) {
        xpos -= 0.5f;
        ypos -= 0.5f;
        double end_angle = std::atan2(ypos, xpos);
        rotate_roll(-(end_angle - rotate_startangle));
        rotate_startangle = end_angle;
    }
}

static void set_texture(unsigned int id) {
    static bool texture_loaded[9] = {false, false, false, false, false, false, false, false, false};
    static const std::string texture_name[9] = {
        "earth1.jpg",
        "earth2.jpg",
        "earth3.jpg",
        "earth4.jpg",
        "moon1.jpg",
        "mars1.jpg",
        "mars2.jpg",
        "jupiter1.jpg",
        "saturn1.jpg",
    };

    if (!texture_loaded[id]) {
        texture_loaded[id] = load_texture(texture_name[id], textures[id]);
    }
    current_texture = id;
}

void handle_key(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
            set_texture(key - GLFW_KEY_1);
        } else if (key == GLFW_KEY_SPACE) { //Reset roll
            reset_roll();
        } else if (key == GLFW_KEY_X) { //Toggle locked/unlocked mode
            if (!lock_north) {
                roll_animation_lock_north = true;
                reset_roll();
            } else {
                lock_north = false;
            }
        } else if (key == GLFW_KEY_ESCAPE) { //Close the window
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

static void place_rotation(GLuint rotation_id) {
    glUniformMatrix3fv(rotation_id, 1, GL_FALSE, rot_mat);
}

int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW!" << std::endl;
        const char* error_description = "";
        int code = glfwGetError(&error_description);
        if (code != GLFW_NO_ERROR) {
            std::cerr << "Received error: " << error_description << std::endl;
        }
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Map projection demo", NULL, NULL);

    if (!window) {
        std::cerr << "Failed to create window!" << std::endl;
        const char* error_description = "";
        int code = glfwGetError(&error_description);
        if (code != GLFW_NO_ERROR) {
            std::cerr << "Received error: " << error_description << std::endl;
        }
        return 1;
    }

    glfwSetKeyCallback(window, handle_key);
    glfwSetCursorPosCallback(window, handle_cursor_position);
    glfwSetMouseButtonCallback(window, handle_mouse_button);
    glfwSetScrollCallback(window, handle_scroll);

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return 1;
    }

    normal = glfwCreateStandardCursor(GLFW_CURSOR_NORMAL);
    grab = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    static const GLfloat main_square_vertices[] = {
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        1.0f, -1.0f, 0.0f,   1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,    1.0f, 0.0f,
        
        1.0f, 1.0f, 0.0f,    1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f
    };

    GLuint main_square;
    GLuint main_square_data;
    glGenVertexArrays(1, &main_square);
    glBindVertexArray(main_square);

    glGenBuffers(1, &main_square_data);
    glBindBuffer(GL_ARRAY_BUFFER, main_square_data);
    glBufferData(GL_ARRAY_BUFFER, sizeof(main_square_vertices), main_square_vertices, GL_STATIC_DRAW);

    Shader equirect;
    if (!load_shader("equirect", equirect)) {
        goto clean;
    }

    GLuint texture_sampler_id; texture_sampler_id = glGetUniformLocation(equirect.program_id, "texture_sampler");
    GLuint rotation_id; rotation_id = glGetUniformLocation(equirect.program_id, "rotation");
    GLuint zoom_id; zoom_id = glGetUniformLocation(equirect.program_id, "zoom");

    set_texture(1);

    while (true) {
        static double last_time = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glUseProgram(equirect.program_id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[current_texture].texture_id);
        glUniform1i(texture_sampler_id, 0);
        glUniform1f(zoom_id, zoom);
        place_rotation(rotation_id);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, main_square_data);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glfwSwapBuffers(window);

        static double last_checked = 0;
        static double worst_dt = 0;
        static int frames_since = 0;
        static double sum_since_checked = 0;
        double nt = glfwGetTime();
        double dt = nt - last_time;
        sum_since_checked += dt;
        last_time = nt;
        if (dt > worst_dt) {
            worst_dt = dt;
        }
        frames_since++;
        if (nt - 10 >= (long long) last_checked) {
            std::cout << "Average time between " << frames_since << " frames: " << sum_since_checked / frames_since << ", FPS: " << (frames_since / sum_since_checked) << std::endl;
            std::cout << "Worst time between frames: " << worst_dt << ", FPS: " << (1 / worst_dt) << std::endl;
            last_checked = nt;
            frames_since = 0;
            worst_dt = 0;
        }

        GLenum last_error;
        while ((last_error = glGetError()) != GL_NO_ERROR) {
            std::cerr << "Got error: " << last_error << std::endl;
        }


        if (roll_animate(nt)) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }

        if (glfwWindowShouldClose(window)) {
            break;
        }
    }

clean:
    glfwDestroyWindow(window);

    glfwTerminate();
}