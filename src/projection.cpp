#include "projection.h"

#include <cmath>

#include <iostream>

// Magic constant
const float PI = 3.141592653589793238462;

bool equirectangular_xy_to_uv(const double x, const double y, double &u, double &v) {
    u = x * PI;
    v = y * PI / 2;
    return !std::isnan(u) && !std::isnan(v);
}

Projection equirectangular = {
    .width = 2,
    .height = 1,
    .shader = "equirect",
    .xy_to_uv = equirectangular_xy_to_uv,
    .prepare_input = nullptr,
    .prepare_output = nullptr,
    .free_input = nullptr,
    .free_output = nullptr
};

static double square(double x) {
    return x * x;
}

bool hammer_xy_to_uv(const double x, const double y, double &u, double &v) {
    double nx = x / std::sqrt(2);
    double ny = y / std::sqrt(2);
    double z_p1 = nx * nx + ny * ny;

    if (z_p1 > 0.5) {
        return false;
    }

    double z = std::sqrt(1 - z_p1);

    u = 2 * std::atan(z * nx * 2 / (2 * z * z - 1));
    v = std::asin(z * ny * 2);

    return !std::isnan(u) && !std::isnan(v);
}

Projection hammer = {
    .width = 2,
    .height = 1,
    .shader = "hammer",
    .xy_to_uv = hammer_xy_to_uv,
    .prepare_input = nullptr,
    .prepare_output = nullptr,
    .free_input = nullptr,
    .free_output = nullptr
};

bool azimuthal_xy_to_uv(const double x, const double y, double &u, double &v) {
    if (x * x + y * y >= 1) {
        return false;
    }
    v = PI / 2 - std::sqrt(x * x + y * y) * PI;
    u = std::atan2(x, -y);
    return !std::isnan(u) && !std::isnan(v);
}

Projection azimuthal = {
    .width = 1,
    .height = 1,
    .shader = "azimuthal",
    .xy_to_uv = &azimuthal_xy_to_uv,
    .prepare_input = nullptr,
    .prepare_output = nullptr,
    .free_input = nullptr,
    .free_output = nullptr
};
