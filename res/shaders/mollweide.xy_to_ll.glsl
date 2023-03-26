bool xy_to_ll(inout vec2 zoomed) {
    zoomed.y = asin(zoomed.y);
    zoomed.x *= PI / cos(zoomed.y);
    
    if (zoomed.x < -PI || zoomed.x > PI) {
        return false;
    }

    //This is necessary to make the projection preserve area
    zoomed.y = asin((2 * zoomed.y + sin(2 * zoomed.y)) / PI);
    return true;
}
