#ifndef MAPS_H
#define MAPS_H

#include <string>

#include "images.h"
#include "projection.h"

struct SphereMap {
    bool loaded;
    std::string texture_name;
    Texture texture;
    Projection *source;
};

bool set_map(unsigned int id);
bool set_map_pack(unsigned int id);
SphereMap* get_current_map();

#endif
