void ll_to_xy(inout vec2 uv) {
    float cy = cos(uv.y);
    float d = sqrt(1 + cy * cos(uv.x / 2))
    uv = vec2(2 * sqrt(2) * cy * sin(uv.x / 2), sqrt(2) * sin(uv.y));
}
