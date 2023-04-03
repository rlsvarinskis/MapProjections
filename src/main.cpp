#include <iostream>
#include <map>
#include <string>
#include <cmath>

#ifdef WIN32
#include "Windows.h"
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "GL/glext.h"
#include "GL/gl.h"

#include "projection.h"
#include "projections/mollweide.h"
#include "mapper.h"
#include "shaders.h"
#include "images.h"
#include "maps.h"

Projection *output_projection;

static bool drag_active = false;
static bool rotate_active = false;
static double drag_startx = 0;
static double drag_starty = 0;
static double rotate_startangle = 0;
static double zoom = 1;

static bool infinite_mode = false;

static GLFWcursor *normal;
static GLFWcursor *grab;

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
    if (h * output_projection->width > w * output_projection->height) {
        // Width is the limiting factor, therefore the real height is smaller than the window's height
        y *= output_projection->width / (double) output_projection->height * h / (double) w;
    } else {
        // Width is the limiting factor, therefore the real width is smaller than the window's width
        x *= output_projection->height / (double) output_projection->width * w / (double) h;
    }
}

void handle_drag_start(GLFWwindow *window) {
    glfwGetCursorPos(window, &drag_startx, &drag_starty);
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    drag_startx /= w;
    drag_starty /= h;
    drag_starty = 1 - drag_starty;
    remap_to_map_xy(drag_startx, drag_starty, w, h);
    if (drag_startx <= -zoom || drag_startx >= zoom || drag_starty <= -zoom || drag_starty >= zoom || !output_projection->xy_to_uv(drag_startx / zoom, drag_starty / zoom, drag_startx, drag_starty)) {
        return;
    }
    drag_active = true;
    glfwSetCursor(window, grab);
}

void handle_drag(GLFWwindow *window, double xpos, double ypos) {
    if (!drag_active) {
        return;
    }

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    xpos /= w;
    ypos /= h;
    ypos = 1 - ypos;
    remap_to_map_xy(xpos, ypos, w, h);
    
    double dx, dy;
    if (xpos <= -zoom || xpos >= zoom || ypos <= -zoom || ypos >= zoom || !output_projection->xy_to_uv(xpos / zoom, ypos / zoom, dx, dy)) {
        return;
    }
    handle_rotation(drag_startx, drag_starty, dx, dy);

    drag_startx = dx;
    drag_starty = dy;
}

void handle_drag_stop(GLFWwindow *window) {
    if (!drag_active) {
        return;
    }

    drag_active = false;
    glfwSetCursor(window, normal);
}

void handle_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            handle_drag_start(window);
        } else {
            handle_drag_stop(window);
        }
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) { //Begin rotate
            rotate_active = true;
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            glfwSetCursor(window, grab);
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            x /= w;
            y /= h;
            y = 1 - y;
            remap_to_map_xy(x, y, w, h);
            
            rotate_startangle = std::atan2(y, x);
        } else { //End rotate
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

    if (rotate_active && !is_locked()) {
        int w, h;
        double x, y;
        glfwGetWindowSize(window, &w, &h);
        x = xpos / w;
        y = ypos / h;
        y = 1 - y;
        double end_angle = std::atan2(y - 0.5f, x - 0.5f);
        rotate_roll(-(end_angle - rotate_startangle));
        rotate_startangle = end_angle;
    }

    if (drag_active) {
        handle_drag(window, xpos, ypos);
    }
}

struct LoadedShader {
    Projection *source;
    Projection *output;
    unsigned int last_width;
    unsigned int last_height;
    Shader shader;

    GLuint texture_sampler_id;
    GLuint zoom_id;
    GLuint scale_id;
    GLuint rotation_matrix_id;
    GLuint infinite_mode_id;
};

std::map<std::string, unsigned int> projection_id;
std::map<unsigned long long, LoadedShader> loaded_shaders;
LoadedShader *current_shader;

static unsigned int get_projection_id(Projection *projection) {
    static int projection_count = 0;

    auto it = projection_id.find(projection->shader);
    unsigned int id = -1;
    if (it == projection_id.end()) {
        id = projection_count++;
        projection_id[projection->shader] = id;
    } else {
        id = it->second;
    }

    return id;
}

static bool update_shader() {
    SphereMap *current_map = get_current_map();
    unsigned int source_id = get_projection_id(current_map->source);
    unsigned int output_id = get_projection_id(output_projection);
    unsigned long long shader_id = (((unsigned long long) source_id) << 32) | output_id;

    LoadedShader *loaded_shader;
    auto it = loaded_shaders.find(shader_id);
    if (it == loaded_shaders.end()) {
        LoadedShader temp;
        loaded_shader = &temp;

        temp.source = current_map->source;
        temp.output = output_projection;
        temp.last_width = 0;
        temp.last_height = 0;

        std::string vertex_shader;
        std::string fragment_shader;
        std::string buffer;

        if (!read_shader("vertex", vertex_shader)) {
            std::cerr << "Failed to load vertex shader!" << std::endl;
            return false;
        }

        if (!read_shader("fragment", fragment_shader)) {
            std::cerr << "Failed to load fragment shader!" << std::endl;
            return false;
        }

        if (!read_shader(current_map->source->shader + ".ll_to_xy", buffer)) {
            std::cerr << "Failed to read " << current_map->source->shader << " source mapping shader!" << std::endl;
            return false;
        }

        fragment_shader += "\n" + buffer;
        buffer.clear();

        if (!read_shader(output_projection->shader + ".xy_to_ll", buffer)) {
            std::cerr << "Failed to read " << output_projection->shader << " output mapping shader!" << std::endl;
            return false;
        }

        fragment_shader += "\n" + buffer;

        if (!load_shader(current_map->source->shader + " to " + output_projection->shader, vertex_shader, fragment_shader, temp.shader)) {
            std::cerr << "Failed to compile generated " << current_map->source->shader << " to " << output_projection->shader << " shader!" << std::endl;
            std::cerr << "Fragment shader dump:" << std::endl;
            std::cerr << fragment_shader << std::endl;
            return false;
        }

        //std::cout << "Loaded shader " << current_map->source->shader + " to " + output_projection->shader << ", code:" << std::endl;
        //std::cout << fragment_shader << std::endl;

        temp.texture_sampler_id = glGetUniformLocation(temp.shader.program_id, "texture_sampler");
        temp.scale_id = glGetUniformLocation(temp.shader.program_id, "scale");
        temp.rotation_matrix_id = glGetUniformLocation(temp.shader.program_id, "rotation");
        temp.zoom_id = glGetUniformLocation(temp.shader.program_id, "zoom");
        temp.infinite_mode_id = glGetUniformLocation(temp.shader.program_id, "infinite_mode");

        loaded_shader = &loaded_shaders[shader_id];
        *loaded_shader = temp;
    } else {
        loaded_shader = &(it->second);
    }

    current_shader = loaded_shader;
    if (current_shader->source->prepare_input) {
        current_shader->source->prepare_input(get_current_map()->texture.width, get_current_map()->texture.height, current_shader->shader.program_id);
    }
    if (current_shader->output->prepare_output) {
        current_shader->output->prepare_output(get_current_map()->texture.width, get_current_map()->texture.height, current_shader->shader.program_id);
    }
    return true;
}

static bool set_projection(Projection *projection) {
    Projection *prev = output_projection;
    output_projection = projection;
    if (!update_shader()) {
        std::cerr << "Failed to load shader for new output projection " << projection->shader << ", going back to " << prev->shader << std::endl;
        output_projection = prev;
        return false;
    }
    return true;
}

static void select_map(unsigned int id) {
    if (!set_map(id)) {
        std::cerr << "Failed to set map " << id << std::endl;
        return;
    }
    if (update_shader()) {
        return;
    }
    if (!set_map_pack(0)) {
        std::cerr << "Failed to reset map pack to 0" << std::endl;
    }
    if (!set_map(0)) {
        std::cerr << "Failed to reset map to 0" << std::endl;
    }
}

static void select_pack(unsigned int id) {
    if (!set_map_pack(id)) {
        std::cerr << "Failed to set map pack " << id << std::endl;
        return;
    }
    if (update_shader()) {
        return;
    }
    if (!set_map_pack(0)) {
        std::cerr << "Failed to reset map pack to 0" << std::endl;
    }
}

void handle_key(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // 1-9 - set map
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
            select_map(key - GLFW_KEY_1);
        }

        // QWERTY - set map pack
        else if (key == GLFW_KEY_Q) { select_pack(0); }
        else if (key == GLFW_KEY_W) { select_pack(1); }
        else if (key == GLFW_KEY_E) { select_pack(2); }
        else if (key == GLFW_KEY_R) { select_pack(3); }
        else if (key == GLFW_KEY_T) { select_pack(4); }
        else if (key == GLFW_KEY_Y) { select_pack(5); }

        // ASDF - set output projection
        else if (key == GLFW_KEY_A) { set_projection(&equirectangular); }
        else if (key == GLFW_KEY_S) { set_projection(&mollweide); }
        else if (key == GLFW_KEY_D) { set_projection(&hammer); }
        else if (key == GLFW_KEY_F) { set_projection(&azimuthal); }
        else if (key == GLFW_KEY_G) { set_projection(&robinson); }

        // Reset roll
        else if (key == GLFW_KEY_SPACE) {
            reset_roll();
        }

        // Toggle locked/unlocked mode
        else if (key == GLFW_KEY_X) {
            toggle_lock();
        }

        // Easter egg to make the projeciton infinite
        else if (key == GLFW_KEY_C) {
            infinite_mode = !infinite_mode;
        }
        
        // Close the window
        else if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

static double last_time;

static void start_frame() {
    last_time = glfwGetTime();
}

static void measure_fps() {
    static double last_checked = 0;
    static double worst_dt = 0;
    static int frames_since = 0;
    static double sum_since_checked = 0;
    double nt = glfwGetTime();
    double dt = nt - last_time;
    //rotate_roll(-dt * 3.141592653589793238462);
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
        sum_since_checked = 0;
    }
}

#define ERR(CODE)                                               \
    CODE                                                        \
    {GLenum last_error;                                         \
    while ((last_error = glGetError()) != GL_NO_ERROR) {        \
        std::cerr << "Executing line:" << std::endl;            \
        std::cerr << #CODE << std::endl;                        \
        std::cerr << "Got error: " << last_error << std::endl;  \
    }}

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
        if (!load_image("./res/icon.png", window_icon)) {
            std::cerr << "Failed to load window icon!" << std::endl;
            goto clean;
        }
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

    prepare_rectangle();

    output_projection = &equirectangular;
    if (!set_map_pack(0)) {
        std::cerr << "Failed to load map pack 0!" << std::endl;
        goto clean;
    }
    if (!update_shader()) {
        std::cerr << "Failed to load default output projection shader " << output_projection->shader << "! Aborting!" << std::endl;
        goto clean;
    }

    while (true) {
        start_frame();

        glClear(GL_COLOR_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        if (width != current_shader->last_width || height != current_shader->last_height) {
            current_shader->last_width = width;
            current_shader->last_height = height;
            glViewport(0, 0, width, height);
            float scale_x = 1;
            float scale_y = 1;
            if (height * output_projection->width > width * output_projection->height) {
                scale_y = output_projection->height / output_projection->width * width / height;
            } else {
                scale_x = output_projection->width / output_projection->height * height / width;
            }
            ERR(glUseProgram(current_shader->shader.program_id);)
            ERR(glUniform2f(current_shader->scale_id, scale_x, scale_y);)
        }

        ERR(glUseProgram(current_shader->shader.program_id);)
        ERR(glActiveTexture(GL_TEXTURE0);)
        ERR(glBindTexture(GL_TEXTURE_2D, get_current_map()->texture.texture_id);)
        ERR(glUniform1i(current_shader->texture_sampler_id, 0);)
        ERR(glUniform1f(current_shader->zoom_id, zoom);)
        ERR(glUniform1i(current_shader->infinite_mode_id, infinite_mode);)
        ERR(render_rectangle(current_shader->rotation_matrix_id);)

        glfwSwapBuffers(window);

        measure_fps();

        GLenum last_error;
        while ((last_error = glGetError()) != GL_NO_ERROR) {
            std::cerr << "Got error: " << last_error << std::endl;
        }

        if (animate_roll(glfwGetTime())) {
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
