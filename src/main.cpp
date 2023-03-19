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

// Latitude
static double ns = 0;
// Longitude
static double ew = 0;

static double roll = 0;

static bool drag_active = false;
static double drag_startx = 0;
static double drag_starty = 0;

static GLFWcursor *normal;
static GLFWcursor *grab;

unsigned int current_texture = 9;
Texture textures[9];

void handle_mouse_button(GLFWwindow* window, int button, int action, int mods) {
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
    }
}

void handle_cursor_position(GLFWwindow* window, double xpos, double ypos) {
    if (drag_active) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        xpos /= w;
        ypos /= h;

        double dx = xpos - drag_startx;
        double dy = ypos - drag_starty;

        ew -= dx * 2;
        ns -= dy;

        drag_startx = xpos;
        drag_starty = ypos;
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

void handle_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
            set_texture(key - GLFW_KEY_1);
        } else if (key == GLFW_KEY_SPACE) { //Reset roll
        } else if (key == GLFW_KEY_X) { //Toggle locked/unlocked mode
            //toggle locked/unlocked
        } else if (key == GLFW_KEY_ESCAPE) { //Close the window
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

static void place_rotation(GLuint rotation_id) {
    float ry = ew * PI;
    float rx = ns * PI;
    float rz = roll * PI;

    float sx = std::sin(rx), cx = std::cos(rx);
    float sy = std::sin(ry), cy = std::cos(ry);
    float sz = std::sin(rz), cz = std::cos(rz);

    //r(z) * r(x) * r(y)
    float rotxy[9] = {
        cy, -sx * sy, cx * sy,
        0, cx, sx,
        -sy, -sx * cy, cx * cy
    };
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
                rot[i * 3 + j] += rotxy[i * 3 + k] * rotz[k * 3 + j];
            }
        }
    }

    glUniformMatrix3fv(rotation_id, 1, GL_FALSE, rot);
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

    set_texture(1);

    while (true) {
        static double last_time = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glUseProgram(equirect.program_id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[current_texture].texture_id);
        glUniform1i(texture_sampler_id, 0);
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
        double nt = glfwGetTime();
        double dt = nt - last_time;
        last_time = nt;
        if (dt > worst_dt) {
            worst_dt = dt;
        }
        frames_since++;
        if (nt - 10 >= (long long) last_checked) {
            std::cout << "Average time between " << frames_since << " frames: " << (nt - last_checked) / frames_since << ", FPS: " << (frames_since / (nt - last_checked)) << std::endl;
            std::cout << "Worst time between frames: " << worst_dt << ", FPS: " << (1 / worst_dt) << std::endl;
            last_checked = nt;
            frames_since = 0;
            worst_dt = 0;
        }

        GLenum last_error;
        while ((last_error = glGetError()) != GL_NO_ERROR) {
            std::cerr << "Got error: " << last_error << std::endl;
        }

        glfwWaitEvents();

        if (glfwWindowShouldClose(window)) {
            break;
        }
    }

clean:
    glfwDestroyWindow(window);

    glfwTerminate();
}