/**
 * The rotation matrix for the sphere.
 * Keep this linear transformation a rotation only, or else all of the code will get messed up.
 */
extern float rot_mat[9];

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
