#include "Map.hpp"

Map::Map(flecs::world *ecs_world)
{
    //--------------------------------------------------------------------------------------
    // Set ECS World for adding map objects
    //--------------------------------------------------------------------------------------
    this->ecs_world = ecs_world;

    //--------------------------------------------------------------------------------------
    // Initialize map rendertarget
    //--------------------------------------------------------------------------------------
    map_target = LoadRenderTexture(512, 512);

    //--------------------------------------------------------------------------------------
    // Parse Map
    //--------------------------------------------------------------------------------------
    map = cute_tiled_load_map_from_file("rockinplants.json", NULL);

    //--------------------------------------------------------------------------------------
    // Load Tileset Textures
    //--------------------------------------------------------------------------------------
    cute_tiled_tileset_t *tileset_ptr = map->tilesets;
    while (tileset_ptr)
    {
        Image tileset_img = LoadImage(tileset_ptr->image.ptr);
        int tile_count = tileset_ptr->tilecount;

        if (tilesets.size() == 0)
            tilesets.push_back(std::make_pair(tile_count, LoadTextureFromImage(tileset_img)));
        else
            tilesets.push_back(std::make_pair(tile_count + tilesets.back().first, LoadTextureFromImage(tileset_img)));

        UnloadImage(tileset_img);

        tileset_ptr = tileset_ptr->next;
    }

    //--------------------------------------------------------------------------------------
    // Render map layers to RenderTexture
    //--------------------------------------------------------------------------------------
    int map_w = map->width;
    int map_h = map->height;

    cute_tiled_layer_t *layer = map->layers;

    BeginTextureMode(map_target);
    while (layer)
    {
        if (std::string("tilelayer") == layer->type.ptr)
        {
            int *data = layer->data;
            int data_count = layer->data_count;

            for (int i = 0; i < map_w; i++)
            {
                for (int j = 0; j < map_h; j++)
                {
                    // Get the tile num for the tile on this layer
                    int tile_data = data[map_w * j + i];
                    if (tile_data == 0)
                        continue;

                    // Figure out the tile's texture
                    Texture2D *tile_tex = NULL;
                    for (auto &tile_pair : tilesets)
                    {
                        if (tile_data <= tile_pair.first)
                        {
                            tile_tex = &(tile_pair.second);
                            break;
                        }
                    }

                    // Draw to the rendertexture
                    Rectangle src_rect = {16.f * ((tile_data - 1) % 23), 16.f * ((tile_data - 1) / 23), 16.f, 16.f};
                    DrawTextureRec(*tile_tex, src_rect, Vector2{i * 16.f, j * 16.f}, WHITE);
                }
            }
        }
        else if (std::string("objectgroup") == layer->type.ptr)
        {
            //--------------------------------------------------------------------------------------
            // Add Solid Bodies
            //--------------------------------------------------------------------------------------
            if (std::string("Collision") == layer->name.ptr)
            {
                cute_tiled_object_t *layer_obj = layer->objects;

                while (layer_obj)
                {
                    flecs::entity solid_e = ecs_world->entity();
                    solid_e.set<plt::Position>({layer_obj->x, layer_obj->y});
                    solid_e.set<plt::Collider>({Rectangle{0, 0, layer_obj->width, layer_obj->height}, c2AABB{0, 0, 0, 0}});
                    solid_e.set<plt::SolidBody>({1});

                    layer_obj = layer_obj->next;
                }
            }
            //--------------------------------------------------------------------------------------
            // Add farmable land
            //--------------------------------------------------------------------------------------
            else if (std::string("Farmable") == layer->name.ptr)
            {
                cute_tiled_object_t *layer_obj = layer->objects;

                while (layer_obj)
                {
                    flecs::entity solid_e = ecs_world->entity();
                    solid_e.set<plt::Farmable>({Rectangle{layer_obj->x, layer_obj->y, layer_obj->width, layer_obj->height}});

                    layer_obj = layer_obj->next;
                }
            }

            //--------------------------------------------------------------------------------------
            // Add objects
            //--------------------------------------------------------------------------------------
            else if (std::string("Objects") == layer->name.ptr)
            {
                cute_tiled_object_t *layer_obj = layer->objects;

                while (layer_obj)
                {
                    // ==================================================
                    // Add the house
                    // ==================================================
                    if (std::string("Home") == layer_obj->name.ptr)
                    {
                        flecs::entity home_e = ecs_world->entity();
                        home_e.set<plt::Home>({Vector2{layer_obj->x, layer_obj->y}});
                        home_e.set<plt::Position>({layer_obj->x, layer_obj->y});
                        home_e.set<plt::Collider>({Rectangle{-19, -10, 38, 10}, c2AABB{0, 0, 0, 0}});
                        home_e.set<plt::SolidBody>({1});
                    }
                    // ==================================================
                    // Add the HomeZone
                    // ==================================================
                    else if (std::string("HomeZone") == layer_obj->name.ptr)
                    {
                        flecs::entity homezone_e = ecs_world->entity();
                        homezone_e.set<plt::HomeZone>({Rectangle{layer_obj->x, layer_obj->y, layer_obj->width, layer_obj->height}});
                    }
                    // ==================================================
                    // Add the BuyZone
                    // ==================================================
                    else if (std::string("BuyZone") == layer_obj->name.ptr)
                    {
                        flecs::entity buyzone_e = ecs_world->entity();
                        buyzone_e.set<plt::BuyZone>({Rectangle{layer_obj->x, layer_obj->y, layer_obj->width, layer_obj->height}});
                    }
                    // ==================================================
                    // Add the player
                    // ==================================================
                    else if (std::string("Spawn") == layer_obj->name.ptr)
                    {
                        flecs::entity player_e = ecs_world->entity();
                        player_e.set<plt::Position>({layer_obj->x, layer_obj->y});
                        player_e.set<plt::Player>({false, plt::PlayerMvnmtState_Idle, 0, 0.1, 0.3, 0, flecs::Empty, plt::PlayerHoldState_Nothing, false, 0});
                        player_e.set<plt::Collider>({Rectangle{-7, -2, 14, 8}, c2AABB{0, 0, 0, 0}});
                        player_e.set<plt::DynamicBody>({1});
                    }
                    else if (std::string("Deco") == layer_obj->name.ptr)
                    {
                        flecs::entity deco_e = ecs_world->entity();
                        deco_e.set<plt::Position>({layer_obj->x, layer_obj->y});
                        deco_e.set<plt::Deco>({(uint8_t)(rand() % 16)});
                    }

                    layer_obj = layer_obj->next;
                }
            }
        }

        layer = layer->next;
    }
    EndTextureMode();
}

Map::~Map()
{
    UnloadRenderTexture(map_target);

    for (auto &tex : tilesets)
        UnloadTexture(tex.second);

    cute_tiled_free_map(map);
}

void Map::draw()
{
    DrawTextureRec(map_target.texture, (Rectangle){0, 0, (float)map_target.texture.width, (float)-map_target.texture.height}, Vector2{0, 0}, WHITE);
}
