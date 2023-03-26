#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Magic constant
const float PI = 3.141592653589793238462;

/**
 * The rotation matrix for the sphere.
 * Keep this linear transformation a rotation only, or else all of the code will get messed up.
 */
float rot_mat[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

void rotate_roll(float roll) {
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

/**
 * Animation parameters
 */
static float roll_animation_amount = 0;
static float roll_animation_start = 0;
static float last_roll_animation_time = 1.0f;
static const float roll_animation_duration = 0.4f;
static bool roll_animation_lock_north = false;

static float ns = 0;
static float ew = 0;

/**
 * Regenerate rot_mat from ns/ew or accumulate all changes to rot_mat
 */
static bool lock_north = false;

/**
 * Begin animating roll rotation to place north up.
 */
void reset_roll() {
    //North vector (0, 1, 0) will go the 2nd column of rot_mat^T
    float roll = std::atan2(-rot_mat[1], rot_mat[4]); //-x / y, so that up is 0 degrees
    roll_animation_amount = roll;
    roll_animation_start = glfwGetTime();
    last_roll_animation_time = 0;
}

static float easeOutQuintic(float time) {
    return 1 - std::pow(1 - time, 5);
}

/**
 * Handle roll animation and return whether the animation needs more frames.
 */
bool animate_roll(float time) {
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
    }
    return true;
}

void toggle_lock() {
    if (!lock_north) {
        roll_animation_lock_north = true;
        reset_roll();
    } else {
        lock_north = false;
    }
}

bool is_locked() {
    return lock_north || roll_animation_lock_north;
}

void handle_rotation(double sx, double sy, double ex, double ey) {
    if (lock_north) {
        ew += ex - sx;
        ns += ey - sy;
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
        rotate_by(0, -sx);
        rotate_by(ey - sy, ex - sx);
        rotate_by(0, sx);
    }
}
