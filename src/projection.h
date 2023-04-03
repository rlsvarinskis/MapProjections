#ifndef PROJECTION_H
#define PROJECTION_H

#include <string>

struct Projection {
    const double width;
    const double height;

    const std::string shader;

    bool (* const (xy_to_uv))(const double x, const double y, double &u, double &v);
    bool (* const (prepare_input))(const unsigned int image_width, const unsigned int image_height, const unsigned int shader_program);
    bool (* const (prepare_output))(const unsigned int screen_width, const unsigned int screen_height, const unsigned int shader_program);
    bool (* const (free_input))();
    bool (* const (free_output))();
};

extern Projection equirectangular;
extern Projection hammer;
extern Projection azimuthal;
extern Projection robinson;

#endif
