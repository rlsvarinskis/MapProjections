#version 330 core

#define PI 3.141592653589793238462f

in vec2 UV;

out vec3 color;

uniform float zoom;
uniform sampler2D texture_sampler;
uniform mat3 rotation;

void main() {
    vec2 zoomed = (UV - vec2(0.5f, 0.5f)) / zoom;
    zoomed.y = asin(-zoomed.y * 2);
    zoomed.x *= 2 * PI / cos(zoomed.y);
    zoomed.y = asin((2 * zoomed.y + sin(2 * zoomed.y)) / PI);
    if (zoomed.x < -PI || zoomed.x > PI) {
        color = vec3(0, 0, 0);
        return;
    }
    vec3 origin = vec3(sin(zoomed.x) * cos(zoomed.y), sin(zoomed.y), cos(zoomed.x) * cos(zoomed.y));

    vec3 dest = rotation * origin;
    vec2 uv = vec2(atan(dest.x, dest.z) / 2, -asin(dest.y)) / PI;
    uv += vec2(0.5f, 0.5f);
    uv -= floor(uv);

    color = texture(texture_sampler, uv).rgb;
}