#ifndef PROJECTION_H
#define PROJECTION_H

#include <string>

struct Projection {
    const double width;
    const double height;

    const std::string shader;

    bool (* const (uv_to_xy))(const double u, const double v, double &x, double &y);
    bool (* const (xy_to_uv))(const double x, const double y, double &u, double &v);
};

extern Projection equirectangular;
extern Projection mollweide;
extern Projection azimuthal;

#endif
