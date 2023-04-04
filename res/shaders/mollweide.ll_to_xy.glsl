uniform sampler1D texture_000_300;
uniform sampler1D texture_300_314;
uniform sampler1D texture_314_pi;
uniform float t000_offset;
uniform float t300_offset;
uniform float t314_offset;

void ll_to_xy(inout vec2 uv) {
    float mul = 1;
    if (uv.y < 0) {
        mul = -1;
        uv.y *= -1;
    }
    uv.y = PI * sin(uv.y);
    if (uv.y <= 3) {
        uv.y = texture(texture_000_300, t000_offset + (1 - 2 * t000_offset) * (uv.y / 3)).r;
    } else if (uv.y <= 3.14) {
        uv.y = texture(texture_300_314, t300_offset + (1 - 2 * t300_offset) * ((uv.y - 3) / 0.14f)).r;
    } else if (uv.y <= PI) {
        uv.y = texture(texture_314_pi, t314_offset + (1 - 2 * t314_offset) * ((uv.y - 3.14f) / (PI - 3.14f))).r;
    }
    uv.y *= mul;

    uv = vec2(uv.x / PI * cos(uv.y), sin(uv.y));
}
