uniform sampler1D l_to_y;
uniform sampler1D l_to_x;
uniform float l_offset;

void ll_to_xy(inout vec2 uv) {
    float mul = 1;
    if (uv.y < 0) {
        mul = -1;
        uv.y = -uv.y;
    }
    float l = 2 * uv.y / PI;
    uv.x = uv.x / PI * texture(l_to_x, l_offset + l * (1 - 2 * l_offset)).r;
    uv.y = mul * texture(l_to_y, l_offset + l * (1 - 2 * l_offset)).r;
}
