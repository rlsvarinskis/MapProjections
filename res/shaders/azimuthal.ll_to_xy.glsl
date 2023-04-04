void ll_to_xy(inout vec2 uv) {
    uv = vec2(sin(uv.x), -cos(uv.x)) * ((PI / 2 - uv.y) / PI);
}
