#pragma once
#include "main.hpp"

class Map
{
private:
    std::vector<std::pair<int, Texture2D>> tilesets;

    flecs::world *ecs_world;

    cute_tiled_map_t *map;

    RenderTexture2D map_target;

public:
    Map(flecs::world *ecs_world);
    ~Map();

    void draw(); 
}; 