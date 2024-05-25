#pragma once
#include "main.hpp"

struct TilesetInfo
{
    cute_tiled_tileset_t info;
    Texture2D tex;
};

class Map
{
private:
    std::vector<TilesetInfo> tilesets_info;

    flecs::world *ecs_world;

    cute_tiled_map_t *map;

    //--------------------------------------------------------------------------------------
    // Map targets
    //--------------------------------------------------------------------------------------

    // Map portion drawn behind everything else
    RenderTexture2D map_target;

public:
    Map(flecs::world *ecs_world);
    ~Map();

    void draw();
};
