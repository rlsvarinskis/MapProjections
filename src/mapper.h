#ifndef MAPPER_H
#define MAPPER_H

void rotate_roll(float roll);

/**
 * Begin animating roll rotation to place north up.
 */
void reset_roll();

/**
 * Handle roll animation and return whether the animation needs more frames.
 */
bool animate_roll(float time);

void toggle_lock();
bool is_locked();
void handle_rotation(double sx, double sy, double ex, double ey);

void prepare_rectangle();
void render_rectangle(GLuint rotation_matrix_id);

#endif
