uniform sampler1D y_to_l;
uniform sampler1D yl_to_x;
uniform float xy_offset;

bool xy_to_ll(inout vec2 zoomed) {
    if (zoomed.y > 1 || zoomed.y < -1) {
        return false;
    }

    float mul = 1;
    if (zoomed.y < 0) {
        mul = -1;
    }

    float l = texture(y_to_l, xy_offset + mul * zoomed.y * (1 - 2 * xy_offset)).r;
    zoomed.y = mul * l * PI / 2;
    zoomed.x /= texture(yl_to_x, xy_offset + l * (1 - 2 * xy_offset)).r;

    if (zoomed.x < -1 || zoomed.x > 1) {
        return false;
    }

    zoomed.x *= PI;
    return true;
}
