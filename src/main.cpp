#include <iostream>
#include <cmath>

#ifdef WIN32
#include "Windows.h"
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "GL/glext.h"
#include "GL/gl.h"

#include "shaders.h"
#include "images.h"
#include "mapper.h"
#include "projection.h"

Projection *projection = /*&equirectangular*//*&mollweide*/&azimuthal;

static bool drag_active = false;
static bool rotate_active = false;
static double drag_startx = 0;
static double drag_starty = 0;
static double rotate_startangle = 0;
static double zoom = 1;

static GLFWcursor *normal;
static GLFWcursor *grab;

unsigned int current_texture = 9;
Texture textures[9];

/**
 * This function remaps window coordinates to the current projection's coordinates, with (-1, -1) to (1, 1) being the rectangle in which the projection is displayed.
 * 
 * @param x x coordinate to remap
 * @param y y coordinate to remap
 * @param w window width
 * @param h window height
*/
static void remap_to_map_xy(double &x, double &y, int w, int h) {
    // If the window's height ratio is larger than than the projection's height ratio
    x = 2 * x - 1;
    y = 2 * y - 1;
    if (h * projection->width > w * projection->height) {
        // Width is the limiting factor, therefore the real height is smaller than the window's height
        y *= projection->width / (double) projection->height * h / (double) w;
    } else {
        // Width is the limiting factor, therefore the real width is smaller than the window's width
        x *= projection->height / (double) projection->width * w / (double) h;
    }
}

void handle_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) { //Begin drag
            glfwGetCursorPos(window, &drag_startx, &drag_starty);
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            drag_startx /= w;
            drag_starty /= h;
            remap_to_map_xy(drag_startx, drag_starty, w, h);
            if (drag_startx < -1 || drag_startx > 1 || drag_starty < -1 || drag_starty > 1 || !projection->xy_to_uv(drag_startx / zoom, drag_starty / zoom, drag_startx, drag_starty)) {
                return;
            }
            drag_active = true;
            glfwSetCursor(window, grab);
        } else if (drag_active) { //End drag
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
            remap_to_map_xy(x, y, w, h);
            
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

void handle_cursor_position(GLFWwindow *window, double xpos, double ypos) {
    if (!drag_active && !rotate_active) {
        return;
    }

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    xpos /= w;
    ypos /= h;

    if (rotate_active && !is_locked()) {
        double end_angle = std::atan2(ypos - 0.5f, xpos - 0.5f);
        rotate_roll(-(end_angle - rotate_startangle));
        rotate_startangle = end_angle;
    }

    if (drag_active) {
        remap_to_map_xy(xpos, ypos, w, h);
        double dx, dy;
        if (!projection->xy_to_uv(xpos / zoom, ypos / zoom, dx, dy)) {
            return;
        }
        handle_rotation(drag_startx, drag_starty, dx, dy);

        drag_startx = dx;
        drag_starty = dy;
    }
}

struct SphereMap {
    bool loaded;
    std::string texture_name;
    Texture texture;
    Projection *source;
};

struct SphereMapPack {
    SphereMap *maps;
    int current_map;
};

SphereMap earth[] = {
    {
        .loaded = false,
        .texture_name = "earth1.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth2.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth3.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth4.jpg",
        .source = &equirectangular
    },
    /*{
        .loaded = false,
        .texture_name = "earth5.jpg",
        .source = &azimuthal
    },
    {
        .loaded = false,
        .texture_name = "earth6.jpg",
        .source = &mollweide
    },
    {
        .loaded = false,
        .texture_name = "earth7.jpg",
        .source = &mollweide
    },
    {
        .loaded = false,
        .texture_name = "earth8.jpg",
        .source = &robinson
    }*/
};
SphereMap moon[] = {
    {
        .loaded = false,
        .texture_name = "moon1.jpg",
        .source = &equirectangular
    }
};
SphereMap mars[] = {
    {
        .loaded = false,
        .texture_name = "mars1.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "mars2.jpg",
        .source = &equirectangular
    }
};
SphereMap jupiter[] = {
    {
        .loaded = false,
        .texture_name = "jupiter1.jpg",
        .source = &equirectangular
    }
};
SphereMap saturn[] = {
    {
        .loaded = false,
        .texture_name = "saturn1.jpg",
        .source = &equirectangular
    }
};
SphereMap universe[] = {
    /*{
        .loaded = false,
        .texture_name = "universe1.png",
        .source = &mollweide
    }*/
};

SphereMapPack maps[] = {
    {earth, 0},
    {moon, 0},
    {mars, 0},
    {jupiter, 0},
    {saturn, 0},
    // {universe, 0}
};
unsigned int current_map;

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
            toggle_lock();
        } else if (key == GLFW_KEY_ESCAPE) { //Close the window
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

static void place_rotation(GLuint rotation_id) {
    glUniformMatrix3fv(rotation_id, 1, GL_FALSE, rot_mat);
}

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
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

    {
        Image window_icon;
        load_image("./res/icon.png", window_icon);
        GLFWimage images[1];
        images[0].width = window_icon.width;
        images[0].height = window_icon.height;
        images[0].pixels = window_icon.data;
        glfwSetWindowIcon(window, 1, images);
        free_image(window_icon);
    }

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
    if (!load_shader(projection->shader, equirect)) {
        std::cerr << "Failed to load shader!" << std::endl;
        goto clean;
    }

    GLuint texture_sampler_id; texture_sampler_id = glGetUniformLocation(equirect.program_id, "texture_sampler");
    GLuint rotation_id; rotation_id = glGetUniformLocation(equirect.program_id, "rotation");
    GLuint zoom_id; zoom_id = glGetUniformLocation(equirect.program_id, "zoom");

    set_texture(1);

    while (true) {
        static double last_time = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);

        int x = 0, y = 0, width, height;
        glfwGetFramebufferSize(window, &width, &height);

        if (height * projection->width > width * projection->height) {
            int nh = width * projection->height / projection->width;
            y = (height - nh) / 2;
            height = nh;
        } else {
            int nw = height * projection->width / projection->height;
            x = (width - nw) / 2;
            width = nw;
        }
        glViewport(x, y, width, height);

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


        if (animate_roll(nt)) {
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

    return 0;
}
