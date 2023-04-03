uniform sampler1D texture_000_300;
uniform sampler1D texture_300_314;
uniform sampler1D texture_314_pi;

void ll_to_xy(inout vec2 uv) {
    float mul = 1;
    if (uv.y < 0) {
        mul = -1;
        uv.y *= -1;
    }
    uv.y = PI * sin(uv.y);
    //TODO: the textures should begin at 0.5 of a pixel, not 0
    //Otherwise small textures get weird clamping at the places where the textures seam
    //You can see what I mean by going into projections/mollweide.cpp and changing the cnt_xyz variables to something like 16
    if (uv.y <= 3) {
        uv.y = texture(texture_000_300, uv.y / 3).r;
    } else if (uv.y <= 3.14) {
        uv.y = texture(texture_300_314, (uv.y - 3) / 0.14f).r;
    } else if (uv.y <= PI) {
        uv.y = texture(texture_314_pi, (uv.y - 3.14f) / (PI - 3.14f)).r;
    }
    uv.y *= mul;

    uv = vec2(uv.x / PI * cos(uv.y), sin(uv.y));
}
