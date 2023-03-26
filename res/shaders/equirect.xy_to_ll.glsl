bool xy_to_ll(inout vec2 zoomed) {
    zoomed.y /= 2;
    zoomed *= PI;
    return true;
}
