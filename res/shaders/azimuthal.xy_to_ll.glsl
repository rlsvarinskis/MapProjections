bool xy_to_ll(inout vec2 zoomed) {
    float l = length(zoomed);
    zoomed = vec2(atan(zoomed.x, -zoomed.y), (0.5f - l) * PI);
    if (l >= 1) {
        return false;
    }
    return true;
}
