#include <string>

struct Projection {
    const unsigned int width;
    const unsigned int height;

    const std::string shader;

    //Info for lock mode (probably only azimuthal equidistant will be different, since it will be centered on 0 longitude 90 latitude)

    bool (* const (uv_to_xy))(const double u, const double v, double &x, double &y);
    bool (* const (xy_to_uv))(const double x, const double y, double &u, double &v);
};

extern Projection equirectangular;
extern Projection mollweide;
extern Projection azimuthal;
