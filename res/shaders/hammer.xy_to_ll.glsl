bool xy_to_ll(inout vec2 zoomed) {
    float nx = zoomed.x / sqrt(2);
    float ny = zoomed.y / sqrt(2);
    float z_p1 = nx * nx + ny * ny;
    float z = sqrt(1 - z_p1);

    zoomed = vec2(2 * atan(z * nx * 2 / (2 * z * z - 1)), asin(z * ny * 2));
    
    if (z_p1 > 0.5) {
        return false;
    }

    return true;
}
