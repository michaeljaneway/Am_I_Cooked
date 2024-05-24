#include "App.hpp"

App::App(int screen_w, int screen_h)
{
    // ==================================================
    // Set screen w and h
    // ==================================================
    this->screen_w = screen_w;
    this->screen_h = screen_h;

    render_colliders = false;
    render_farmable = false;
    render_positions = false;

    has_collected_bag = false;
    has_planted_seed = false;
    has_collected_plant = false;
    has_sold_plant = false;
    has_bought_seeds = false;

    // Load Font (https://ggbot.itch.io/kaph-font)
    kaph_font = LoadFontEx("Kaph-Regular.ttf", 128, 0, 250);

    inv_rot = {0.0, 20, 20, true, -1, 1, EaseInOutCubic};
    inv_scale = {0.0, 40, 40, true, 0.95, 1, EaseInOutCubic};
    text_y_add = {0.0, 5, 5, true, 0, 10, EaseInOutCubic};

    occupied_plant_area = std::vector<std::vector<flecs::entity_t>>(32, std::vector<flecs::entity_t>(32, flecs::Empty));

    std::vector<std::string> plant_names({
        "Rose",
        "Pointy",
        "Daisy",
        "Marigold",
        "Delphinium",
        "Lavender",
        "Primrose",
        "Tulip",
    });

    for (uint8_t i = 0; i < 8; i++)
    {
        plant_money_info.push_back({(plt::PlantType)i, plant_names[i], (i + 1) * 2, (i + 1) * 3});
    }

    // ==================================================
    // Initialize ECS World
    // ==================================================
    ecs_world = std::make_unique<flecs::world>();
    initSystems();

    // ==================================================
    // Initialize the Map
    // ==================================================
    map = std::make_unique<Map>(ecs_world.get());

    // ==================================================
    // Load crop textures
    // ==================================================
    {
        Image plant_tileset_img = LoadImage("objects.png");
        plants_tex = LoadTextureFromImage(plant_tileset_img);
        UnloadImage(plant_tileset_img);
    }

    // ==================================================
    // Load the tileset textures
    // ==================================================
    {
        Image tileset_img = LoadImage("tileset.png");
        tileset_tex = LoadTextureFromImage(tileset_img);
        UnloadImage(tileset_img);
    }

    // ==================================================
    // Load the player textures
    // ==================================================

    // Load fwd walking
    {
        Image walking_front_img = LoadImage("character_walking_front.png");
        player_fwd_tex = LoadTextureFromImage(walking_front_img);
        UnloadImage(walking_front_img);
    }

    // Load l and r from the same file
    {
        Image walking_lr_img = LoadImage("character_walking_side.png");
        player_r_tex = LoadTextureFromImage(walking_lr_img);
        ImageFlipHorizontal(&walking_lr_img);
        player_l_tex = LoadTextureFromImage(walking_lr_img);
        UnloadImage(walking_lr_img);
    }

    // Load back walking
    {
        Image walking_back_img = LoadImage("character_walking_back.png");
        player_bck_tex = LoadTextureFromImage(walking_back_img);
        UnloadImage(walking_back_img);
    }

    // ==================================================
    // Add a Plant Bag
    // ==================================================
    flecs::entity bag_e = ecs_world->entity();
    bag_e.set<plt::Position>({300, 250});
    bag_e.set<plt::PlantBag>({plt::PlantType_Rose});
}

App::~App()
{
    UnloadFont(kaph_font);
    UnloadTexture(plants_tex);
}

void App::initSystems()
{
    flecs::system player_system = ecs_world->system<plt::Position, plt::Player>()
                                      .kind(flecs::PreUpdate)
                                      .each([&](flecs::entity e, plt::Position &pos, plt::Player &player)
                                            {
                                                PlayerSystem(e, pos, player); //
                                            });

    flecs::system collision_system = ecs_world->system<plt::Position, plt::Collider>()
                                         .kind(flecs::PreUpdate)
                                         .each([&](flecs::entity e, plt::Position &pos, plt::Collider &coll)
                                               {
                                                   CollisionSystem(e, pos, coll); //
                                               });

    flecs::system dynamic_body_system = ecs_world->system<plt::Position, plt::Collider, plt::DynamicBody>()
                                            .kind(flecs::PreUpdate)
                                            .each([&](flecs::entity e, plt::Position &pos, plt::Collider &coll, plt::DynamicBody &dyn)
                                                  {
                                                      DynamicBodySystem(e, pos, coll); //
                                                  });

    flecs::system plant_system = ecs_world->system<plt::Position, plt::Plant>()
                                     .kind(flecs::PreUpdate)
                                     .each([&](flecs::entity e, plt::Position &pos, plt::Plant &plant)
                                           {
                                               PlantSystem(e, pos, plant); //
                                           });

    flecs::system render_system = ecs_world->system()
                                      .kind(flecs::PostUpdate)
                                      .iter([&](flecs::iter &it)
                                            {
                                                RenderSystem(); //
                                            });
}

void App::runFrame()
{
    ecs_world->progress();
}

int AABBtoPoint(c2AABB A, c2v B)
{
    int d0 = B.x < A.min.x;
    int d1 = B.y < A.min.y;
    int d2 = B.x > A.max.x;
    int d3 = B.y > A.max.y;
    return !(d0 | d1 | d2 | d3);
}

void App::PlayerSystem(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    if (player.in_buy_menu)
        return;

    //--------------------------------------------------------------------------------------
    // Handle player movement and walk cycle
    //--------------------------------------------------------------------------------------

    Vector2 dist = {0, 0};
    plt::PlayerMvnmtState prev_move_state = player.move_state;
    player.move_state = plt::PlayerMvnmtState_Idle;

    if (IsKeyDown(KeyboardKey::KEY_S))
    {
        dist.y += 1;
        player.move_state = plt::PlayerMvnmtState_Forward;
    }
    if (IsKeyDown(KeyboardKey::KEY_A))
    {
        dist.x -= 1;
        player.move_state = plt::PlayerMvnmtState_Left;
    }
    if (IsKeyDown(KeyboardKey::KEY_D))
    {
        dist.x += 1;
        player.move_state = plt::PlayerMvnmtState_Right;
    }
    if (IsKeyDown(KeyboardKey::KEY_W))
    {
        dist.y -= 1;
        player.move_state = plt::PlayerMvnmtState_Back;
    }

    dist = Vector2Normalize(dist);
    // dist = Vector2Scale(dist, 1.5);
    dist = Vector2Add(Vector2{pos.x, pos.y}, dist);

    pos.x = dist.x;
    pos.y = dist.y;

    if (player.move_state == plt::PlayerMvnmtState_Idle || player.move_state != prev_move_state)
    {
        player.current_frame = 0;
        player.time_till_fchange = player.time_per_fchange;
    }
    else
    {
        player.time_till_fchange -= ecs_world->delta_time();

        if (player.time_till_fchange <= 0)
        {
            player.current_frame = (player.current_frame + 1) % 6;
            player.time_till_fchange = player.time_per_fchange;
        }
    }

    //--------------------------------------------------------------------------------------
    // Check if we're on farmable land
    //--------------------------------------------------------------------------------------
    int curr_tile_x = int(pos.x) / 16;
    int curr_tile_y = int(pos.y) / 16;

    player.on_farmable_land = false;
    flecs::filter<plt::Farmable> farmable_f = ecs_world->filter<plt::Farmable>();
    farmable_f.each([&](flecs::entity e, plt::Farmable &farmable)
                    {
                        c2AABB farm_area = {farmable.area.x, farmable.area.y, farmable.area.x + farmable.area.width, farmable.area.y + farmable.area.height};
                        if (!AABBtoPoint(farm_area, c2v{pos.x, pos.y}))
                            return;

                        player.on_farmable_land = true; //
                    });

    //--------------------------------------------------------------------------------------
    // Switch actions based on what we're holding
    //--------------------------------------------------------------------------------------

    switch (player.holding_state)
    {
    case plt::PlayerHoldState_Nothing:
    {
        //--------------------------------------------------------------------------------------
        // Check if we're near a plant bag we can grab
        //--------------------------------------------------------------------------------------
        flecs::filter<plt::Position, plt::PlantBag> bag_f = ecs_world->filter<plt::Position, plt::PlantBag>();
        bag_f.each([&](flecs::entity bag_e, plt::Position &bag_pos, plt::PlantBag &bag)
                   {
                       if (player.holding_state != plt::PlayerHoldState_Nothing)
                           return;

                       c2AABB bag_area = {bag_pos.x - 12, bag_pos.y - 12, bag_pos.x + 12, bag_pos.y + 12};
                       if (!AABBtoPoint(bag_area, c2v{pos.x, pos.y}))
                           return;

                       has_collected_bag = true;
                       bag_e.remove<plt::Position>();
                       player.item = bag_e.id();
                       player.holding_state = plt::PlayerHoldState_SeedBag;
                       //
                   });

        //--------------------------------------------------------------------------------------
        // Check if we want to harvest a plant
        //--------------------------------------------------------------------------------------
        if (player.on_farmable_land && IsKeyDown(KEY_E) && occupied_plant_area[curr_tile_x][curr_tile_y] != flecs::Empty)
        {
            flecs::entity_t plant_id = occupied_plant_area[curr_tile_x][curr_tile_y];
            flecs::entity plant_ent = ecs_world->get_alive(plant_id);

            plt::Plant *plant_comp = plant_ent.get_mut<plt::Plant>();

            if (plant_comp->harvestable)
            {
                // Move the respective flower into our inventory
                plant_ent.remove<plt::Position>();
                player.item = plant_id;
                player.holding_state = plt::PlayerHoldState_Flower;

                has_collected_plant = true;

                // Free up space for another flower
                occupied_plant_area[curr_tile_x][curr_tile_y] = flecs::Empty;
            }
        }
        //--------------------------------------------------------------------------------------
        // Check if we can buy
        //--------------------------------------------------------------------------------------
        else if (IsKeyDown(KEY_E))
        {
            bool on_buyzone = false;
            flecs::filter<plt::BuyZone> buyzone_f = ecs_world->filter<plt::BuyZone>();
            buyzone_f.each([&](flecs::entity e, plt::BuyZone &bzone)
                           {
                               c2AABB buyzone_area = {bzone.zone.x, bzone.zone.y, bzone.zone.x + bzone.zone.width, bzone.zone.y + bzone.zone.height};
                               if (!AABBtoPoint(buyzone_area, c2v{pos.x, pos.y}))
                                   return;

                               on_buyzone = true; //
                           });

            if (!on_buyzone)
                break;

            player.in_buy_menu = true;
        }
    }
    break;
    case plt::PlayerHoldState_SeedBag:
    {
        //--------------------------------------------------------------------------------------
        // Check to plant a plant
        //--------------------------------------------------------------------------------------
        if (player.on_farmable_land && IsKeyDown(KEY_E) && occupied_plant_area[curr_tile_x][curr_tile_y] == flecs::Empty)
        {
            int x = curr_tile_x * 16 + 8;
            int y = curr_tile_y * 16 + 8;

            flecs::entity bag_ent = ecs_world->get_alive(player.item);
            plt::PlantBag *bag_comp = bag_ent.get_mut<plt::PlantBag>();

            flecs::entity plant_e = ecs_world->entity();
            plant_e.set<plt::Position>({(float)x, (float)y});
            plant_e.set<plt::Plant>({bag_comp->plant_type, 3, 3, 0, false});

            has_planted_seed = true;

            bag_ent.destruct();
            player.holding_state = plt::PlayerHoldState_Nothing;

            occupied_plant_area[curr_tile_x][curr_tile_y] = plant_e.id();
        }
    }
    break;
    case plt::PlayerHoldState_Flower:
    {
        //--------------------------------------------------------------------------------------
        // Return plant to home for more seeds
        //--------------------------------------------------------------------------------------
        if (IsKeyDown(KEY_E))
        {
            bool on_homezone = false;
            flecs::filter<plt::HomeZone> homezone_f = ecs_world->filter<plt::HomeZone>();
            homezone_f.each([&](flecs::entity e, plt::HomeZone &hzone)
                            {
                                c2AABB homezone_area = {hzone.zone.x, hzone.zone.y, hzone.zone.x + hzone.zone.width, hzone.zone.y + hzone.zone.height};
                                if (!AABBtoPoint(homezone_area, c2v{pos.x, pos.y}))
                                    return;

                                on_homezone = true; //
                            });

            if (!on_homezone)
                break;

            flecs::entity plant_ent = ecs_world->get_alive(player.item);
            plt::Plant *plant_comp = plant_ent.get_mut<plt::Plant>();

            has_sold_plant = true;

            player.money += plant_money_info[plant_comp->plant_type].sell_value;

            plant_ent.destruct();
            player.holding_state = plt::PlayerHoldState_Nothing;
        }
    }
    break;

    default:
        break;
    }
}

void App::CollisionSystem(flecs::entity e, plt::Position &pos, plt::Collider &coll)
{
    coll.body.min.x = pos.x + coll.bounds.x;
    coll.body.min.y = pos.y + coll.bounds.y;

    coll.body.max.x = pos.x + coll.bounds.x + coll.bounds.width;
    coll.body.max.y = pos.y + coll.bounds.y + coll.bounds.height;
}

void App::DynamicBodySystem(flecs::entity e, plt::Position &pos, plt::Collider &coll)
{
    flecs::filter<plt::Position, plt::Collider, plt::SolidBody> solid_body_f = ecs_world->filter<plt::Position, plt::Collider, plt::SolidBody>();
    solid_body_f.each([&](flecs::entity s_e, plt::Position &s_pos, plt::Collider &s_coll, plt::SolidBody &sol)
                      {
                          c2Manifold man;
                          c2AABBtoAABBManifold(coll.body, s_coll.body, &man);
                          if (man.count == 0)
                              return;

                          pos.x -= man.n.x;
                          pos.y -= man.n.y; //
                      });
}

void App::PlantSystem(flecs::entity e, plt::Position &pos, plt::Plant &plant)
{
    plant.time_to_next_stage -= ecs_world->delta_time();

    if (plant.time_to_next_stage <= 0 && plant.current_growth_stage < 3)
    {
        plant.time_to_next_stage = plant.time_per_stage;
        plant.current_growth_stage += 1;
    }

    if (plant.current_growth_stage == 3)
    {
        plant.harvestable = true;
    }
}

bool compSPR(plt::SPR i, plt::SPR j)
{
    return (i.y_level < j.y_level);
}

// Used to update plt::LoopingEase structures
void processLoopingEase(plt::LoopingEase &le, float dt)
{
    le.cur_time_left -= dt;

    if (le.cur_time_left < 0)
    {
        le.cur_time_left = le.set_time;
        le.increasing = !le.increasing;
    }

    easingFunction ease_func = getEasingFunction(le.func);
    float percentage = 1.0 - (le.cur_time_left / le.set_time);
    float progress = ease_func(percentage);

    if (le.increasing)
        le.val = Lerp(le.min_val, le.max_val, progress);
    else
        le.val = Lerp(le.max_val, le.min_val, progress);
}

void App::RenderSystem()
{
    processLoopingEase(inv_rot, ecs_world->delta_time());
    processLoopingEase(inv_scale, ecs_world->delta_time());
    processLoopingEase(text_y_add, ecs_world->delta_time());

    BeginDrawing();
    ClearBackground(RAYWHITE);

    //--------------------------------------------------------------------------------------
    // Render Map
    //--------------------------------------------------------------------------------------
    map->draw();

    //--------------------------------------------------------------------------------------
    // Clear previous frame render orders
    //--------------------------------------------------------------------------------------
    render_orders.clear();

    //--------------------------------------------------------------------------------------
    // Render Home
    //--------------------------------------------------------------------------------------
    flecs::filter<plt::Home> home_f = ecs_world->filter<plt::Home>();
    home_f.each([&](flecs::entity e, plt::Home &home)
                {
                    Vector2 draw_pos = {home.pos.x - 24, home.pos.y - 48};
                    render_orders.push_back({home.pos.y, tileset_tex, Rectangle{272.f, 48, 48, 48}, draw_pos, WHITE});
                    //
                });

    //--------------------------------------------------------------------------------------
    // Render Plant Bags
    //--------------------------------------------------------------------------------------
    flecs::filter<plt::Position, plt::PlantBag> bag_f = ecs_world->filter<plt::Position, plt::PlantBag>();
    bag_f.each([&](flecs::entity e, plt::Position &pos, plt::PlantBag &bag)
               {
                   Vector2 draw_pos = {pos.x - 8, pos.y - 10};
                   render_orders.push_back({pos.y, plants_tex, Rectangle{16.f * bag.plant_type, 0, 16, 16}, draw_pos, WHITE});
                   //
               });

    //--------------------------------------------------------------------------------------
    // Render Player
    //--------------------------------------------------------------------------------------
    flecs::filter<plt::Position, plt::Player> player_f = ecs_world->filter<plt::Position, plt::Player>();
    player_f.each([&](flecs::entity e, plt::Position &pos, plt::Player &player)
                  {
                      if (player.on_farmable_land)
                      {
                          Rectangle highlight_rect = {std::floor(pos.x / 16.f) * 16.f,
                                                      std::floor(pos.y / 16.f) * 16.f,
                                                      16,
                                                      16};
                          DrawRectangleRec(highlight_rect, ColorAlpha(WHITE, 0.3));
                      }

                      // Subtract a bit more than 32 from y so sprite is a bit above the collidersa
                      Vector2 draw_pos = {pos.x - 40, pos.y - 32 - 11};

                      switch (player.move_state)
                      {
                      case plt::PlayerMvnmtState_Idle:
                          render_orders.push_back({pos.y, player_fwd_tex, Rectangle{0, 0, 80, 64}, draw_pos, WHITE});
                          break;
                      case plt::PlayerMvnmtState_Left:
                          render_orders.push_back({pos.y, player_l_tex, Rectangle{80.f * player.current_frame, 0, 80, 64}, draw_pos, WHITE});
                          break;
                      case plt::PlayerMvnmtState_Right:
                          render_orders.push_back({pos.y, player_r_tex, Rectangle{80.f * player.current_frame, 0, 80, 64}, draw_pos, WHITE});
                          break;
                      case plt::PlayerMvnmtState_Back:
                          render_orders.push_back({pos.y, player_bck_tex, Rectangle{80.f * player.current_frame, 0, 80, 64}, draw_pos, WHITE});
                          break;
                      case plt::PlayerMvnmtState_Forward:
                          render_orders.push_back({pos.y, player_fwd_tex, Rectangle{80.f * player.current_frame, 0, 80, 64}, draw_pos, WHITE});
                          break;
                      default:
                          break;
                      }
                      //
                  });

    //--------------------------------------------------------------------------------------
    // Render Plants
    //--------------------------------------------------------------------------------------
    flecs::filter<plt::Position, plt::Plant> plant_f = ecs_world->filter<plt::Position, plt::Plant>();
    plant_f.each([&](flecs::entity e, plt::Position &pos, plt::Plant &plant)
                 {
                     float x, y, w, h;
                     Vector2 draw_pos = {pos.x - 8, pos.y - 8};

                     x = 16 * plant.plant_type;
                     w = 16;

                     if (plant.current_growth_stage < 2)
                     {
                         y = 32 + 16 * plant.current_growth_stage;
                         h = 16;
                     }
                     else
                     {
                         y = 64 + 32 * (plant.current_growth_stage - 2);
                         h = 32;
                         draw_pos.y -= 16;
                     }

                     render_orders.push_back({pos.y, plants_tex, Rectangle{x, y, w, h}, draw_pos, WHITE});
                     //
                 });

    //--------------------------------------------------------------------------------------
    // Render Decor
    //--------------------------------------------------------------------------------------
    flecs::filter<plt::Position, plt::Deco> deco_f = ecs_world->filter<plt::Position, plt::Deco>();
    deco_f.each([&](flecs::entity e, plt::Position &pos, plt::Deco &deco)
                {
                    Rectangle target = {16.f * deco.type, 80.f, 16, 16};
                    Vector2 draw_pos = {pos.x - 8, pos.y - 10};

                    render_orders.push_back({pos.y, tileset_tex, target, draw_pos, WHITE});
                    //
                });

    //--------------------------------------------------------------------------------------
    // Render all textures with y-level sorting
    //--------------------------------------------------------------------------------------

    std::sort(render_orders.begin(), render_orders.end(), compSPR);

    for (auto &spr : render_orders)
    {
        DrawTextureRec(spr.tex, spr.source, spr.position, spr.color);
    }

    //--------------------------------------------------------------------------------------
    // Render Inventory
    //--------------------------------------------------------------------------------------

    bool is_player_in_buy_menu = false;
    {
        flecs::filter<plt::Position, plt::Player> inv_f = ecs_world->filter<plt::Position, plt::Player>();
        inv_f.each([&](flecs::entity e, plt::Position &pos, plt::Player &player)
                   {
                       renderPlayerInventory(e, pos, player);
                       renderPlayerMoney(e, pos, player);

                       if (player.in_buy_menu)
                       {
                           is_player_in_buy_menu = true;
                           renderPlayerBuyMenu(e, pos, player);
                       }
                       //
                   });
    }

    GuiSetFont(kaph_font);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_TOP);
    GuiSetStyle(DEFAULT, TEXT_SIZE, kaph_font.baseSize / 4);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, 30);

    float shadow_offset = 1.5;

    if (!is_player_in_buy_menu)
    {
        if (!has_collected_bag)
        {
            drawTutorialText("Collect the Bag");

            flecs::filter<plt::Position, plt::PlantBag> bag_f = ecs_world->filter<plt::Position, plt::PlantBag>();
            bag_f.each([&](flecs::entity bag_e, plt::Position &bag_pos, plt::PlantBag &bag)
                       {
                           drawAttentionArrow(Vector2{bag_pos.x, bag_pos.y}); //
                       });
        }
        else if (!has_planted_seed)
        {
            drawTutorialText("Plant the seed\nin the shirt");
            drawAttentionArrow(Vector2{screen_w / 2.f, screen_h / 2.f});
        }
        else if (!has_collected_plant)
        {
            flecs::filter<plt::Position, plt::Plant> plant_f = ecs_world->filter<plt::Position, plt::Plant>();
            plant_f.each([&](flecs::entity bag_e, plt::Position &plt_pos, plt::Plant &plant)
                         {
                             if (plant.current_growth_stage != 3)
                                 drawTutorialText("LET IT GROW");
                             else
                                 drawTutorialText("Harvest the plant");

                             drawAttentionArrow(Vector2{plt_pos.x, plt_pos.y}); //
                         });
        }
        else if (!has_sold_plant)
        {
            drawTutorialText("Sell the plant");

            flecs::filter<plt::Position, plt::Home> bag_f = ecs_world->filter<plt::Position, plt::Home>();
            bag_f.each([&](flecs::entity bag_e, plt::Position &home_pos, plt::Home &home)
                       {
                           drawAttentionArrow(Vector2{home_pos.x, home_pos.y}); //
                       });
        }
        else if (!has_bought_seeds)
        {
            drawTutorialText("Buy more seeds");

            flecs::filter<plt::BuyZone> bz_f = ecs_world->filter<plt::BuyZone>();
            bz_f.each([&](flecs::entity bz_e, plt::BuyZone &bz)
                      {
                          drawAttentionArrow(Vector2{bz.zone.x + bz.zone.width / 2, bz.zone.y + bz.zone.height / 2}); //
                      });
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // DEBUG RENDER SETTINGS
    //
    //--------------------------------------------------------------------------------------

    // GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    // GuiToggle(Rectangle{512 - 10 - 100, 10, 100, 20}, "Render Colliders", &render_colliders);
    // GuiToggle(Rectangle{512 - 10 - 100, 40, 100, 20}, "Render Farmable", &render_farmable);
    // GuiToggle(Rectangle{512 - 10 - 100, 70, 100, 20}, "Render Positions", &render_positions);

    //--------------------------------------------------------------------------------------
    // Render Colliders (DEBUG)
    //--------------------------------------------------------------------------------------
    if (render_colliders)
    {
        flecs::filter<plt::Position, plt::Collider> collider_f = ecs_world->filter<plt::Position, plt::Collider>();
        collider_f.each([&](flecs::entity e, plt::Position &pos, plt::Collider &coll)
                        {
                            Rectangle collider_rect = {coll.body.min.x, coll.body.min.y, coll.body.max.x - coll.body.min.x, coll.body.max.y - coll.body.min.y};
                            DrawRectangleLinesEx(collider_rect, 1, BLACK);
                            //
                        });
    }

    //--------------------------------------------------------------------------------------
    // Render Farmable Land (DEBUG)
    //--------------------------------------------------------------------------------------
    if (render_farmable)
    {
        flecs::filter<plt::Farmable> farmable_f = ecs_world->filter<plt::Farmable>();
        farmable_f.each([&](flecs::entity e, plt::Farmable &farmable)
                        {
                            DrawRectangleLinesEx(farmable.area, 1, RED);
                            //
                        });
    }

    //--------------------------------------------------------------------------------------
    // Render Positions (DEBUG)
    //--------------------------------------------------------------------------------------
    if (render_positions)
    {
        flecs::filter<plt::Position> pos_f = ecs_world->filter<plt::Position>();
        pos_f.each([&](flecs::entity e, plt::Position &pos)
                   {
                       DrawCircleV(Vector2{pos.x, pos.y}, 2, WHITE);
                       //
                   });
    }

    EndDrawing();
}

void App::drawAttentionArrow(Vector2 target)
{
    const float shadow_offset = 1.5;

    DrawTriangle(Vector2{target.x + shadow_offset, target.y - 30 + text_y_add.val + shadow_offset},
                 Vector2{target.x + 10 + shadow_offset, target.y - 70 + text_y_add.val + shadow_offset},
                 Vector2{target.x - 10 + shadow_offset, target.y - 70 + text_y_add.val + shadow_offset}, ColorAlpha(BLACK, 0.7));

    DrawTriangle(Vector2{target.x, target.y - 30 + text_y_add.val},
                 Vector2{target.x + 10, target.y - 70 + text_y_add.val},
                 Vector2{target.x - 10, target.y - 70 + text_y_add.val}, YELLOW);
}

void App::drawTutorialText(std::string text)
{
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(ColorAlpha(BLACK, 0.9)));
    GuiLabel(Rectangle{screen_w / 2.f - 200 + 2, screen_h / 2.f - 240 + text_y_add.val + 2, 400, 200}, text.c_str());
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(MAROON));
    GuiLabel(Rectangle{screen_w / 2.f - 200, screen_h / 2.f - 240 + text_y_add.val, 400, 200}, text.c_str());
}

void App::renderPlayerMoney(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    GuiSetFont(kaph_font);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_MIDDLE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, kaph_font.baseSize / 2);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, 30);

    std::string money_str = "$" + std::to_string(player.money);

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(ColorAlpha(BLACK, 0.9)));
    GuiLabel(Rectangle{10.f + 2, 374.f + 64 + 2, 354.f, 64}, money_str.c_str());
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(MAROON));
    GuiLabel(Rectangle{10.f, 374.f + 64, 354.f, 64}, money_str.c_str());
}

void App::renderPlayerInventory(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Draw Background Rectangle
    DrawRectangleRec(Rectangle{374.f, 374.f, 128, 128}, ColorAlpha(WHITE, 0.4));

    if (player.holding_state == plt::PlayerHoldState_Nothing)
        return;

    flecs::entity item = ecs_world->get_alive(player.item);

    // Target Rectangle
    Rectangle target_rec = {384.f + 54, 384.f + 54, 108 * inv_scale.val, 108 * inv_scale.val};
    Vector2 origin = {target_rec.width / 2, target_rec.height / 2};

    switch (player.holding_state)
    {
    case plt::PlayerHoldState_SeedBag:
    {
        plt::PlantBag *bag = item.get_mut<plt::PlantBag>();
        DrawTexturePro(plants_tex, Rectangle{16.f * bag->plant_type, 0, 16, 16}, target_rec, origin, inv_rot.val, WHITE);
    }
    break;
    case plt::PlayerHoldState_Flower:
    {
        plt::Plant *plant = item.get_mut<plt::Plant>();
        DrawTexturePro(plants_tex, Rectangle{16.f * plant->plant_type, 16, 16, 16}, target_rec, origin, inv_rot.val, WHITE);
    }
    break;

    default:
        break;
    }
}

void App::renderPlayerBuyMenu(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    Font prev_font = GuiGetFont();

    // Draw shop background
    Rectangle menu_rec = Rectangle{10.f, 70.f, screen_w - 20.f, 294.f};
    DrawRectangleRec(menu_rec, ColorAlpha(WHITE, 0.7));

    // Draw shop title
    GuiSetFont(kaph_font);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_TOP);
    GuiSetStyle(DEFAULT, TEXT_SIZE, kaph_font.baseSize / 3);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, 30);

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(ColorAlpha(BLACK, 0.9)));
    GuiLabel(Rectangle{menu_rec.x + 10 + 2, menu_rec.y + 10 + 2, menu_rec.width - 20.f, 30}, "SHOP");
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(MAROON));
    GuiLabel(Rectangle{menu_rec.x + 10, menu_rec.y + 10, menu_rec.width - 20.f, 30}, "SHOP");

    // Draw Buy
    GuiSetFont(GuiGetFont());
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(MAROON));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, GuiDefaultProperty::TEXT_SIZE - 3);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, GuiDefaultProperty::TEXT_LINE_SPACING);

    if (GuiButton(Rectangle{menu_rec.x + 10, menu_rec.y + 10, 50.f, 25}, "Exit"))
    {
        player.in_buy_menu = false;
    }

    // Draw the rectangles for each
    for (uint8_t i = 0; i < 8; i++)
    {
        Rectangle seed_rec = {20.f + 246 * (i % 2), 125.f + 60 * (i / 2), 226, 45};

        DrawRectangleRec(seed_rec, Color{0xD8, 0xE8, 0xBC, 0xFF});

        plt::PlantMoneyInfo curr_plt_info = plant_money_info[i];

        // Render Plant Info
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_TOP);

        std::string buy_str = "Buy Val: " + std::to_string(curr_plt_info.buy_value);
        std::string sell_str = "Sell Val: " + std::to_string(curr_plt_info.sell_value);
        GuiLabel(Rectangle{seed_rec.x + 5, seed_rec.y + 5, 100, 30}, curr_plt_info.name.c_str());

        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiDefaultProperty::TEXT_SIZE - 5);
        GuiLabel(Rectangle{seed_rec.x + 5, seed_rec.y + 20, 100, 30}, buy_str.c_str());
        GuiLabel(Rectangle{seed_rec.x + 5, seed_rec.y + 30, 100, 30}, sell_str.c_str());

        Rectangle target_rec = {seed_rec.x + seed_rec.width - 32 - 8, seed_rec.y + 7, 32, 32};

        if (player.money >= curr_plt_info.buy_value)
        {
            if (GuiButton(target_rec, ""))
            {
                flecs::entity bag_e = ecs_world->entity();
                bag_e.set<plt::PlantBag>({curr_plt_info.plant_type});

                player.money -= curr_plt_info.buy_value;
                has_bought_seeds = true;

                player.holding_state = plt::PlayerHoldState_SeedBag;
                player.item = bag_e.id();
                player.in_buy_menu = false;
            }

            DrawTexturePro(plants_tex, Rectangle{16.f * i, 0, 16, 16}, target_rec, Vector2{0, 0}, 0, WHITE);
        }
        else
        {
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_MIDDLE);
            GuiSetStyle(DEFAULT, TEXT_SIZE, GuiDefaultProperty::TEXT_SIZE);
            GuiLabel(target_rec, "$");
        }
    }

    GuiSetFont(prev_font);
}

void App::drawPulseRect(Rectangle pulse_rec)
{
    DrawRectangleLinesEx(pulse_rec, 2, ColorAlpha(YELLOW, inv_scale.val));
}