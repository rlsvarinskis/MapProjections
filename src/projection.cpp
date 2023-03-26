#include "projection.h"

#include <cmath>

// Magic constant
const float PI = 3.141592653589793238462;

bool equirectangular_uv_to_xy(const double u, const double v, double &x, double &y) {
    return false;
}

bool equirectangular_xy_to_uv(const double x, const double y, double &u, double &v) {
    u = x * PI;
    v = y * PI / 2;
    return !std::isnan(u) && !std::isnan(v);
}

Projection equirectangular = {
    .width = 2,
    .height = 1,
    .shader = "equirect",
    .uv_to_xy = equirectangular_uv_to_xy,
    .xy_to_uv = equirectangular_xy_to_uv
};

bool mollweide_uv_to_xy(const double u, const double v, double &x, double &y) {
    return false;
}

bool mollweide_xy_to_uv(const double x, const double y, double &u, double &v) {
    v = std::asin(y);
    u = x * PI / std::cos(v);
    v = std::asin((2 * v + std::sin(2 * v)) / PI);
    if (u <= -PI || u >= PI) {
        return false;
    }
    return !std::isnan(u) && !std::isnan(v);
}

// A useful paper explaining the Mollweide projection:
// http://master.grad.hr/hdgg/kog_stranica/kog15/2Lapaine-KoG15.pdf
Projection mollweide = {
    .width = 2,
    .height = 1,
    .shader = "mollweide",
    .uv_to_xy = &mollweide_uv_to_xy,
    .xy_to_uv = &mollweide_xy_to_uv
};

bool azimuthal_uv_to_xy(const double u, const double v, double &x, double &y) {
    return false;
}

bool azimuthal_xy_to_uv(const double x, const double y, double &u, double &v) {
    if (x * x + y * y >= 1) {
        return false;
    }
    v = -PI / 2 + std::sqrt(x * x + y * y) * PI;
    u = std::atan2(x, y);
    return !std::isnan(u) && !std::isnan(v);
}

Projection azimuthal = {
    .width = 1,
    .height = 1,
    .shader = "azimuthal",
    .uv_to_xy = &azimuthal_uv_to_xy,
    .xy_to_uv = &azimuthal_xy_to_uv
};
