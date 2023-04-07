#version 330 core

#define PI 3.141592653589793238462f

in vec2 UV;

out vec3 color;

uniform vec2 scale;
uniform vec2 uv_scale;
uniform float zoom;
uniform sampler2D texture_sampler;
uniform mat3 rotation;
uniform bool infinite_mode;

bool xy_to_ll(inout vec2 zoomed);
void ll_to_xy(inout vec2 uv);

bool is_outside(vec2 uv, vec2 a, vec2 b) {
    vec2 test = step(a, uv) - step(b, uv);
    return (test.x * test.y) == 0;
}

void main() {
    vec2 zoomed = (UV * 2 - vec2(1, 1)) / scale / zoom;

    if (infinite_mode) {
        xy_to_ll(zoomed);
    } else if (is_outside(zoomed, vec2(-1, -1), vec2(1, 1)) || !xy_to_ll(zoomed)) {
        color = vec3(0, 0, 0);
        return;
    }

    vec3 origin = vec3(sin(zoomed.x) * cos(zoomed.y), sin(zoomed.y), -cos(zoomed.x) * cos(zoomed.y));
    vec3 dest = rotation * origin;

    vec2 uv = vec2(atan(dest.x, -dest.z), asin(dest.y));

    ll_to_xy(uv);

    uv = (uv + vec2(1, 1)) / 2;
    uv -= floor(uv);

    uv.y = 1 - uv.y;

    color = texture(texture_sampler, uv * uv_scale).rgb;
}
