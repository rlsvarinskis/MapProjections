#include "maps.h"

#include <string>

#include "images.h"
#include "projection.h"
#include "projections/mollweide.h"

static SphereMap earth[] = {
    {
        .loaded = false,
        .texture_name = "earth1.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth2.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth3.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth4.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "earth5.jpg",
        .source = &azimuthal
    },
    {
        .loaded = false,
        .texture_name = "earth6_pow2.jpg",
        .source = &mollweide
    },
    {
        .loaded = false,
        .texture_name = "earth7.jpg",
        .source = &mollweide
    },
    /*{
        .loaded = false,
        .texture_name = "earth8.jpg",
        .source = &robinson
    }*/
};
static SphereMap moon[] = {
    {
        .loaded = false,
        .texture_name = "moon1.jpg",
        .source = &equirectangular
    }
};
static SphereMap mars[] = {
    {
        .loaded = false,
        .texture_name = "mars1.jpg",
        .source = &equirectangular
    },
    {
        .loaded = false,
        .texture_name = "mars2.jpg",
        .source = &equirectangular
    }
};
static SphereMap jupiter[] = {
    {
        .loaded = false,
        .texture_name = "jupiter1.jpg",
        .source = &equirectangular
    }
};
static SphereMap saturn[] = {
    {
        .loaded = false,
        .texture_name = "saturn1.jpg",
        .source = &equirectangular
    }
};
static SphereMap universe[] = {
    {
        .loaded = false,
        .texture_name = "universe1.jpg",
        .source = &mollweide
    }
};

struct SphereMapPack {
    SphereMap *maps;
    int count;
    int current_map;
};

#define MAP_PACK(x) {x, sizeof(x) / sizeof(*x), 0}

static SphereMapPack map_packs[] = {
    MAP_PACK(earth),
    MAP_PACK(moon),
    MAP_PACK(mars),
    MAP_PACK(jupiter),
    MAP_PACK(saturn),
    MAP_PACK(universe),
};
static unsigned int current_map_pack;

static bool set_map(unsigned int pack, unsigned int id) {
    if (id >= map_packs[pack].count) {
        return false;
    }
    SphereMap &sm = map_packs[pack].maps[id];
    if (!sm.loaded) {
        if (!(sm.loaded = load_texture(sm.texture_name, sm.texture))) {
            return false;
        }
    }
    map_packs[pack].current_map = id;
    return true;
}

bool set_map(unsigned int id) {
    return set_map(current_map_pack, id);
}

bool set_map_pack(unsigned int id) {
    if (id >= sizeof(map_packs) / sizeof(*map_packs)) {
        return false;
    }

    if (set_map(id, map_packs[id].current_map)) {
        current_map_pack = id;
        return true;
    }

    return false;
}

SphereMap* get_current_map() {
    SphereMapPack &pack = map_packs[current_map_pack];
    return &pack.maps[pack.current_map];
}
