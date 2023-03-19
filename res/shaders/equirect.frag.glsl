#version 330 core

#define PI 3.141592653589793238462f

in vec2 UV;

out vec3 color;

uniform float zoom;
uniform sampler2D texture_sampler;
uniform mat3 rotation;

void main() {
    vec2 zoomed = (UV - vec2(0.5f, 0.5f)) / zoom + vec2(0.5f, 0.5f);
    float x = (zoomed.x * 2 - 1.0f) * PI;
    vec3 origin = vec3(sin(x) * sin(zoomed.y * PI), -cos(zoomed.y * PI), cos(x) * sin(zoomed.y * PI));

    vec3 dest = rotation * origin;
    vec2 uv = vec2(atan(dest.x, dest.z), -acos(dest.y)) / PI;
    uv.x = (uv.x + 1) / 2;
    uv -= floor(uv);

    color = texture(texture_sampler, uv).rgb;
}