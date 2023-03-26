bool xy_to_ll(inout vec2 zoomed) {
    if (length(zoomed) >= 1) {
        return false;
    }
    zoomed = vec2(atan(zoomed.x, -zoomed.y), (0.5f - length(zoomed)) * PI);
    return true;
}
