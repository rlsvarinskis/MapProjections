#version 330 core

#define PI 3.141592653589793238462f

in vec2 UV;

out vec3 color;

uniform sampler2D texture_sampler;
uniform mat3 rotation;

void main() {
    float x = (UV.x * 2 - 1.0f) * PI;
    vec3 origin = vec3(sin(x) * sin(UV.y * PI), -cos(UV.y * PI), cos(x) * sin(UV.y * PI));

    vec3 dest = rotation * origin;
    vec2 uv = vec2(atan(dest.x, dest.z), -acos(dest.y)) / PI;
    uv.x = (uv.x + 1) / 2;
    uv -= floor(uv);

    color = texture(texture_sampler, uv).rgb;
}