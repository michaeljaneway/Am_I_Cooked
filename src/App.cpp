#include "App.hpp"

// ==================================================
// Utility Functions
// ==================================================

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

int AABBtoPoint(c2AABB A, c2v B)
{
    int d0 = B.x < A.min.x;
    int d1 = B.y < A.min.y;
    int d2 = B.x > A.max.x;
    int d3 = B.y > A.max.y;
    return !(d0 | d1 | d2 | d3);
}

bool compSPR(plt::SPR i, plt::SPR j)
{
    return (i.y_level < j.y_level);
}

c2AABB rectToAABB(Rectangle rec)
{
    return c2AABB{rec.x, rec.y, rec.x + rec.width, rec.y + rec.height};
}

Rectangle AABBtoRect(c2AABB rec)
{
    return Rectangle{rec.min.x, rec.min.y, rec.max.x - rec.min.x, rec.max.y - rec.min.y};
}

void setGuiTextStyle(Font f, int color, int h_align, int v_align, int size, int spacing)
{
    GuiSetFont(f);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, color);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, h_align);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, v_align);
    GuiSetStyle(DEFAULT, TEXT_SIZE, size);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, spacing);
}

// ==================================================
// Order Functions
// ==================================================
void addDishToOrder(plt::Order &o, plt::Dish dish)
{
    o.dishes.push_back(dish);
    o.indicies.push_back(std::make_pair(0, o.dishes.size() - 1));
}

void addIngredientToOrder(plt::Order &o, plt::Ingredient ing)
{
    o.ingredients.push_back(ing);
    o.indicies.push_back(std::make_pair(1, o.ingredients.size() - 1));
}


// ==================================================
// App
// ==================================================
App::App(int screen_w, int screen_h)
{
    // Set screen w and h
    this->screen_w = screen_w;
    this->screen_h = screen_h;

    time_counter = 0;

    render_colliders = false;
    render_positions = false;

    is_audio_initialized = false;

    lookout_font = LoadFontEx("fonts/Lookout 7.ttf", 128, 0, 250);
    fear_font = LoadFontEx("fonts/Fear 11.ttf", 128, 0, 250);

    inv_rot = {0.0, 20, 20, true, -1, 1, EaseInOutCubic};
    inv_scale = {0.0, 40, 40, true, 0.95, 1, EaseInOutCubic};
    text_y_add = {0.0, 5, 5, true, 0, 10, EaseInOutCubic};

    game_state = plt::GameState_MainMenu;
    prev_game_state = plt::GameState_MainMenu;

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
    // Load textures
    // ==================================================

    // Load the player texture
    {
        Image player_img = LoadImage("chef_ghost_strip.png");
        player_tex = LoadTextureFromImage(player_img);
        UnloadImage(player_img);
    }
    // Load the food texture
    {
        Image meals_img = LoadImage("meals.png");
        meals_tex = LoadTextureFromImage(meals_img);
        UnloadImage(meals_img);
    }

    initFood();
}

App::~App()
{
    UnloadTexture(player_tex);
    UnloadTexture(meals_tex);

    for (auto &track : game_music)
        UnloadMusicStream(track);
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

    flecs::system customer_system = ecs_world->system()
                                        .kind(flecs::PreUpdate)
                                        .iter([&](flecs::iter &it)
                                              {
                                                  CustomerSystem(); //
                                              });

    flecs::system render_system = ecs_world->system()
                                      .kind(flecs::PostUpdate)
                                      .iter([&](flecs::iter &it)
                                            {
                                                RenderSystem(); //
                                            });
}

void App::initFood()
{
    // Vegetables
    ingredients.push_back({"Melon", {0, 64}, false, plt::Whole});
    ingredients.push_back({"Carrot", {32, 64}, false, plt::Whole});
    ingredients.push_back({"White Carrot", {64, 64}, false, plt::Whole});
    ingredients.push_back({"Potato", {128, 64}, false, plt::Whole});
    ingredients.push_back({"Yam", {224, 64}, false, plt::Whole});
    ingredients.push_back({"Purple Yam", {320, 64}, false, plt::Whole});
    ingredients.push_back({"Tomato", {384, 64}, false, plt::Whole});

    ingredients.push_back({"Chicken Leg", {0, 512}, true, plt::Whole});
    ingredients.push_back({"Sausage", {64, 512}, true, plt::Whole});

    ingredients.push_back({"Corn", {448, 64}, false, plt::Whole});
    ingredients.push_back({"Onion", {480, 64}, false, plt::Whole});
    ingredients.push_back({"Red Onion", {512, 64}, false, plt::Whole});
    ingredients.push_back({"Purple Onion", {544, 64}, false, plt::Whole});
    ingredients.push_back({"Green Pepper", {576, 64}, false, plt::Whole});
    ingredients.push_back({"Red Pepper", {608, 64}, false, plt::Whole});
    ingredients.push_back({"Orange Pepper", {640, 64}, false, plt::Whole});

    ingredients.push_back({"Bacon", {96, 512}, true, plt::Whole});
    ingredients.push_back({"Flank", {128, 512}, true, plt::Whole});

    ingredients.push_back({"Yellow Pepper", {672, 64}, false, plt::Whole});
    ingredients.push_back({"Brussel Sprouts", {736, 64}, false, plt::Whole});
    ingredients.push_back({"Cauliflower", {768, 64}, false, plt::Whole});
    ingredients.push_back({"Broccoli", {800, 64}, false, plt::Whole});
    ingredients.push_back({"Squash", {864, 64}, false, plt::Whole});
    ingredients.push_back({"Cucumber", {896, 64}, false, plt::Whole});
    ingredients.push_back({"Radish", {928, 64}, false, plt::Whole});

    ingredients.push_back({"Meatballs", {160, 512}, true, plt::Whole});
    ingredients.push_back({"Steak", {224, 512}, true, plt::Whole});

    ingredients.push_back({"Turnip", {960, 64}, false, plt::Whole});
    ingredients.push_back({"Apple", {800, 512}, false, plt::Whole});
    ingredients.push_back({"Orange", {832, 512}, false, plt::Whole});
    ingredients.push_back({"Pineapple", {896, 512}, false, plt::Whole});
    ingredients.push_back({"Strawberry", {928, 512}, false, plt::Whole});
    ingredients.push_back({"Kiwi", {992, 512}, false, plt::Whole});

    // Bowls
    // dishes.push_back({"Small Bowl", plt::DishType_Bowl, {0, 0}, plt::BowlFillType_None});
    // dishes.push_back({"Medium Bowl", plt::DishType_Bowl, {32, 0}, plt::BowlFillType_None});
    dishes.push_back({"Large Bowl", plt::DishType_Bowl, {64, 0}, plt::BowlFillType_None});

    // Plates
    // dishes.push_back({"Small Plate", plt::DishType_Plate, {96, 0}, plt::BowlFillType_None});
    // dishes.push_back({"Medium Plate", plt::DishType_Plate, {128, 0}, plt::BowlFillType_None});
    dishes.push_back({"Large Plate", plt::DishType_Plate, {160, 0}, plt::BowlFillType_None});

    // Fills
    bowl_fills.push_back(Vector2{128, 32});
    bowl_fills.push_back(Vector2{192, 32});
    bowl_fills.push_back(Vector2{256, 32});
    bowl_fills.push_back(Vector2{320, 32});
}

void App::runFrame()
{
    ecs_world->progress();
}

void App::PlayerSystem(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    if (player.cooking_zone != plt::CookingZone_None)
        return;

    //--------------------------------------------------------------------------------------
    // Handle player movement and walk cycle
    //--------------------------------------------------------------------------------------

    Vector2 dist = {0, 0};
    plt::PlayerMvnmtState prev_move_state = player.move_state;

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
    dist = Vector2Scale(dist, 4);
    dist = Vector2Add(Vector2{pos.x, pos.y}, dist);

    pos.x = dist.x;
    pos.y = dist.y;

    player.time_till_fchange -= ecs_world->delta_time();

    if (player.time_till_fchange <= 0)
    {
        player.current_frame = (player.current_frame + 1) % 4;
        player.time_till_fchange = player.time_per_fchange;
    }

    plt::CookingZoneType new_zone_type = plt::CookingZone_None;

    if (IsKeyDown(KEY_E))
    {
        flecs::filter<plt::CookingZone> cooking_zone_f = ecs_world->filter<plt::CookingZone>();
        cooking_zone_f.each([&](flecs::entity e, plt::CookingZone &c_zone)
                            {
                                if (!AABBtoPoint(rectToAABB(c_zone.zone), c2v{pos.x, pos.y}))
                                    return;

                                new_zone_type = c_zone.type; //
                            });
    }

    //--------------------------------------------------------------------------------------
    // Switch actions based on what we're holding
    //--------------------------------------------------------------------------------------

    switch (new_zone_type)
    {
    case plt::CookingZone_None:
        break;

    // Only go into the bag if you are NOT holding anything
    case plt::CookingZone_Bag:
        if (player.holding_type == plt::PlayerHoldingType_None)
            player.cooking_zone = plt::CookingZone_Bag;
        break;

    // Only go into the dishes cupboard if you are NOT holding anything
    case plt::CookingZone_Dishes:
        if (player.holding_type == plt::PlayerHoldingType_None)
            player.cooking_zone = plt::CookingZone_Dishes;
        break;

    // Only go into the sink if you are holding an EMPTY bowl
    case plt::CookingZone_Sink:
        if (player.holding_type == plt::PlayerHoldingType_Dish)
        {
            flecs::entity dish = ecs_world->get_alive(player.item);
            plt::Dish *dish_info = dish.get_mut<plt::Dish>();
            if (dish_info->type == plt::DishType_Bowl && dish_info->fill == plt::BowlFillType_None)
            {
                player.cooking_zone = plt::CookingZone_Sink;
            }
        }
        break;

    // Only go into the cutting board menu if:
    // Holding a Whole, non-cookable ingredient
    case plt::CookingZone_CuttingBoard:
        if (player.holding_type == plt::PlayerHoldingType_Ingredient)
        {
            flecs::entity ing_e = ecs_world->get_alive(player.item);
            plt::Ingredient *ing_info = ing_e.get_mut<plt::Ingredient>();

            if (!ing_info->cookable && ing_info->state == plt::Whole)
            {
                player.cooking_zone = plt::CookingZone_CuttingBoard;
            }
        }
        break;

    // Only go into the trash if you ARE holding something
    case plt::CookingZone_Trash:
        // If we're holding an item, throw it out
        if (player.holding_type != plt::PlayerHoldingType_None)
        {
            ecs_world->get_alive(player.item).destruct();
            player.holding_type = plt::PlayerHoldingType_None;
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
                          // Collision detection and mainfold generation
                          c2Manifold m;
                          c2AABBtoAABBManifold(coll.body, s_coll.body, &m);

                          // Leave if there's no collision
                          if (m.count == 0)
                              return;

                          // Resolve the collision if there is one
                          Vector2 n = {m.n.x, m.n.y};
                          for (int i = 0; i < m.count; i++)
                          {
                              Vector2 p = {m.contact_points[i].x, m.contact_points[i].y};
                              float d = m.depths[i];
                              pos.x -= n.x * d;
                              pos.y -= n.y * d;
                          }

                          //
                      });
}

void App::CustomerSystem()
{
    // If we just started a new cooking round
    if (prev_game_state != game_state)
    {
        // switch (expression)
        // {
        // case /* constant-expression */:
        //     /* code */
        //     break;

        // default:
        //     break;
        // }
    }

    prev_game_state = game_state;
}

//--------------------------------------------------------------------------------------
// Handling Game Music
//--------------------------------------------------------------------------------------

void App::handleGameMusic()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !is_audio_initialized)
    {
        is_audio_initialized = true;
        InitAudioDevice();

        // Add the music
        game_music.push_back(LoadMusicStream("music/jazzfunk.mp3"));
        game_music.push_back(LoadMusicStream("music/nokia.mp3"));
        game_music.push_back(LoadMusicStream("music/dance1.mp3"));
        game_music.push_back(LoadMusicStream("music/churchcombat.mp3"));
        game_music.push_back(LoadMusicStream("music/devil.mp3"));
        game_music.push_back(LoadMusicStream("music/New Sunrise.mp3"));
    }

    switch (game_state)
    {
    case plt::GameState_MainMenu:
        playGameMusic(game_music[plt::GameMusic_MainMenu]);
        break;

    case plt::GameState_Day1Intro:
    case plt::GameState_Day2Intro:
    case plt::GameState_Day3Intro:
        playGameMusic(game_music[plt::GameMusic_Devil]);
        break;

    case plt::GameState_Day1:
        playGameMusic(game_music[plt::GameMusic_Day1]);
        break;

    case plt::GameState_Day2:
        playGameMusic(game_music[plt::GameMusic_Day2]);
        break;

    case plt::GameState_Day3:
        playGameMusic(game_music[plt::GameMusic_Day3]);
        break;

    case plt::GameState_Outro:
        playGameMusic(game_music[plt::GameMusic_Ascension]);
        break;

    default:
        break;
    }
}

void App::playGameMusic(Music &mus)
{
    // Return if the audio device is not ready
    if (!IsAudioDeviceReady())
    {
        return;
    }

    if (!IsMusicStreamPlaying(mus))
    {
        // Pause the playing music track
        for (auto &track : game_music)
            if (IsMusicStreamPlaying(track))
                StopMusicStream(track);

        // Start playing the new song
        PlayMusicStream(mus);
    }
    else
    {
        UpdateMusicStream(mus);
    }
}

//--------------------------------------------------------------------------------------
// Render System
//--------------------------------------------------------------------------------------
void App::RenderSystem()
{
    // Speedrun time counter
    time_counter += ecs_world->delta_time();

    processLoopingEase(inv_rot, ecs_world->delta_time());
    processLoopingEase(inv_scale, ecs_world->delta_time());
    processLoopingEase(text_y_add, ecs_world->delta_time());

    // Handle music
    // handleGameMusic();

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Main menu
    if (game_state == plt::GameState_MainMenu)
    {
        ClearBackground(Color{0x2B, 0x26, 0x27, 0xFF});

        setGuiTextStyle(lookout_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        if (GuiButton(Rectangle{screen_w * 0.25f, 250, screen_w - (screen_w * 0.5f), 50}, "PLAY"))
            game_state = plt::GameState_Day1Intro;

        setGuiTextStyle(fear_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_TOP, lookout_font.baseSize, 90);
        GuiLabel(Rectangle{0, 10, (float)screen_w, 250}, "AM I\nCooked??");

        EndDrawing();
        return;
    }

    //--------------------------------------------------------------------------------------
    // Render Map
    //--------------------------------------------------------------------------------------
    map->draw();

    //--------------------------------------------------------------------------------------
    // Clear previous frame render orders
    //--------------------------------------------------------------------------------------
    render_orders.clear();

    //--------------------------------------------------------------------------------------
    // Render Animated Player
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

                      // Subtract a bit more than 32 from y so sprite is a bit above the colliders
                      Vector2 draw_pos = {round(pos.x - 16), round(pos.y - 32)};

                      const Color ghost_color = ColorAlpha(WHITE, 0.8);

                      switch (player.move_state)
                      {
                      case plt::PlayerMvnmtState_Left:
                          render_orders.push_back({pos.y, player_tex, Rectangle{32.f * (player.current_frame + 12), 0, 32, 32}, draw_pos, ghost_color});
                          break;
                      case plt::PlayerMvnmtState_Right:
                          render_orders.push_back({pos.y, player_tex, Rectangle{32.f * (player.current_frame + 4), 0, 32, 32}, draw_pos, ghost_color});
                          break;
                      case plt::PlayerMvnmtState_Back:
                          render_orders.push_back({pos.y, player_tex, Rectangle{32.f * (player.current_frame + 8), 0, 32, 32}, draw_pos, ghost_color});
                          break;
                      case plt::PlayerMvnmtState_Forward:
                          render_orders.push_back({pos.y, player_tex, Rectangle{32.f * player.current_frame, 0, 32, 32}, draw_pos, ghost_color});
                          break;
                      default:
                          break;
                      }
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
    // Render GUI
    //--------------------------------------------------------------------------------------

    flecs::filter<plt::Position, plt::Player> inv_f = ecs_world->filter<plt::Position, plt::Player>();
    inv_f.each([&](flecs::entity e, plt::Position &pos, plt::Player &player)
               {
                   switch (player.cooking_zone)
                   {
                   case plt::CookingZone_None:
                       renderPlayerInventory(e, pos, player);
                       break;
                   case plt::CookingZone_Bag:
                       renderBagMenu(e, pos, player);
                       break;
                   case plt::CookingZone_Dishes:
                       renderDishMenu(e, pos, player);
                       break;
                   case plt::CookingZone_Sink:
                       renderSinkMenu(e, pos, player);
                       break;
                   case plt::CookingZone_CuttingBoard:
                       renderCuttingBoardMenu(e, pos, player);
                       break;

                   default:
                       break;
                   }
                   //
               });

    //--------------------------------------------------------------------------------------
    // Render Tutorial
    //--------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------
    //
    // DEBUG RENDER SETTINGS
    //
    //--------------------------------------------------------------------------------------

    // GuiToggle(Rectangle{screen_w - 10.f - 100, 10, 100, 20}, "Render Colliders", &render_colliders);
    // GuiToggle(Rectangle{screen_w - 10.f - 100, 40, 100, 20}, "Render Positions", &render_positions);
    // GuiSpinner(Rectangle{screen_w - 10.f - 100, 70, 100, 20}, "", (int *)&game_state, 0, (int)plt::GameState_Outro, false);

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
    // Render Positions (DEBUG)
    //--------------------------------------------------------------------------------------
    if (render_positions)
    {
        flecs::filter<plt::Position> pos_f = ecs_world->filter<plt::Position>();
        pos_f.each([&](flecs::entity e, plt::Position &pos)
                   {
                       DrawCircleV(Vector2{pos.x, pos.y}, 4, ORANGE);
                       DrawCircleV(Vector2{pos.x, pos.y}, 3, RED);
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

void App::drawPulseRect(Rectangle pulse_rec)
{
    DrawRectangleLinesEx(pulse_rec, 2, ColorAlpha(YELLOW, inv_scale.val));
}

void App::renderPlayerInventory(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Draw Background Rectangle
    Rectangle inv_rect = Rectangle{screen_w - 110.f, screen_h - 110.f, 100, 100};
    DrawRectangleRec(inv_rect, ColorAlpha(WHITE, 0.3));

    if (player.holding_type == plt::PlayerHoldingType_None)
        return;

    // Get the item the player is holding
    flecs::entity item = ecs_world->get_alive(player.item);

    // Target Rectangle (within a margin of the inv_rect)
    Rectangle target_rec = {inv_rect.x + 10.f, inv_rect.y + 10.f, inv_rect.width - 20.f, inv_rect.height - 20.f};
    Vector2 origin = {target_rec.width / 2, target_rec.height / 2};

    // Switch based on what type of item we're holding
    switch (player.holding_type)
    {
    // Render an ingredient item
    case plt::PlayerHoldingType_Ingredient:
    {
        plt::Ingredient *item_ing = item.get_mut<plt::Ingredient>();

        DrawTexturePro(meals_tex, {item_ing->pos.x, item_ing->pos.y + 32.f * item_ing->state, 32, 32}, target_rec, {0.f, 0.f}, 0, WHITE);
    }
    break;
    // Render a dish item
    case plt::PlayerHoldingType_Dish:
    {
        plt::Dish *item_ing = item.get_mut<plt::Dish>();

        DrawTexturePro(meals_tex, {item_ing->pos.x, item_ing->pos.y, 32, 32}, target_rec, {0.f, 0.f}, 0, WHITE);

        if (item_ing->fill != plt::BowlFillType_None)
        {
            int fill_int = ((int)item_ing->fill) - 1;
            DrawTexturePro(meals_tex, {bowl_fills[fill_int].x, bowl_fills[fill_int].y, 32, 32}, target_rec, {0.f, 0.f}, 0, WHITE);
        }
    }
    break;

    default:
        break;
    }
}

void App::renderBagMenu(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Menu background
    Rectangle menu_rec = Rectangle{10.f, 10.f, screen_w - 20.f, screen_h - 20.f};
    DrawRectangleRec(menu_rec, ColorAlpha(WHITE, 0.7));

    // Shop Title
    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10 + 1, menu_rec.y + 10 + 1, menu_rec.width - 20.f, 30}, "Ingredients");
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10, menu_rec.y + 10, menu_rec.width - 20.f, 30}, "Ingredients");

    // Exit Button
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 4, 30);
    if (GuiButton(Rectangle{menu_rec.x + 10, menu_rec.y + 10, 100.f, 35}, "Exit"))
        player.cooking_zone = plt::CookingZone_None;

    // Draw ingredient buttons
    int i = 0;
    for (auto ing : ingredients)
    {
        // Rectangle where this ingredient will be drawn
        Rectangle ing_rec = {menu_rec.x + 10 + 66 * (i % 9), menu_rec.y + 70 + 66 * (i / 9), 64, 64};

        if (GuiButton(ing_rec, ""))
        {
            flecs::entity ing_e = ecs_world->entity();
            ing_e.set<plt::Ingredient>(ing);

            player.holding_type = plt::PlayerHoldingType_Ingredient;
            player.item = ing_e.id();
            player.cooking_zone = plt::CookingZone_None;
        }

        DrawTexturePro(meals_tex, {ing.pos.x, ing.pos.y, 32, 32}, ing_rec, {0.f, 0.f}, 0, WHITE);
        i++;
    }
}

void App::renderDishMenu(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Menu background
    Rectangle menu_rec = Rectangle{10.f, 10.f, screen_w - 20.f, screen_h - 20.f};
    DrawRectangleRec(menu_rec, ColorAlpha(WHITE, 0.7));

    // Shop Title
    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10 + 1, menu_rec.y + 10 + 1, menu_rec.width - 20.f, 30}, "Dishes");
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10, menu_rec.y + 10, menu_rec.width - 20.f, 30}, "Dishes");

    // Exit Button
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 4, 30);
    if (GuiButton(Rectangle{menu_rec.x + 10, menu_rec.y + 10, 100.f, 35}, "Exit"))
        player.cooking_zone = plt::CookingZone_None;

    // Draw ingredient buttons
    int i = 0;
    for (auto dish : dishes)
    {
        // Rectangle where this ingredient will be drawn
        Rectangle dish_rec = {menu_rec.x + 10 + 66 * (i % 9), menu_rec.y + 70 + 66 * (i / 9), 64, 64};

        if (GuiButton(dish_rec, ""))
        {
            flecs::entity dish_e = ecs_world->entity();
            dish_e.set<plt::Dish>(dish);

            player.holding_type = plt::PlayerHoldingType_Dish;
            player.item = dish_e.id();
            player.cooking_zone = plt::CookingZone_None;
        }

        DrawTexturePro(meals_tex, {dish.pos.x, dish.pos.y, 32, 32}, dish_rec, {0.f, 0.f}, 0, WHITE);
        i++;
    }
}

void App::renderSinkMenu(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Menu background
    Rectangle menu_rec = Rectangle{10.f, 10.f, screen_w - 20.f, screen_h - 20.f};
    DrawRectangleRec(menu_rec, ColorAlpha(WHITE, 0.7));

    // Shop Title
    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10 + 1, menu_rec.y + 10 + 1, menu_rec.width - 20.f, 30}, "Fill Bowl");
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10, menu_rec.y + 10, menu_rec.width - 20.f, 30}, "Fill Bowl");

    // Exit Button
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 4, 30);
    if (GuiButton(Rectangle{menu_rec.x + 10, menu_rec.y + 10, 100.f, 35}, "Exit"))
        player.cooking_zone = plt::CookingZone_None;

    // Draw ingredient buttons
    for (int i = 0; i < bowl_fills.size(); i++)
    {
        // Rectangle where this ingredient will be drawn
        Rectangle fill_rec = {menu_rec.x + 10 + 66 * (i % 9), menu_rec.y + 70 + 66 * (i / 9), 64, 64};

        if (GuiButton(fill_rec, ""))
        {
            flecs::entity dish = ecs_world->get_alive(player.item);
            plt::Dish *dish_info = dish.get_mut<plt::Dish>();
            dish_info->fill = (plt::BowlFillType)(i + 1);

            player.cooking_zone = plt::CookingZone_None;
        }

        DrawTexturePro(meals_tex, {bowl_fills[i].x, bowl_fills[i].y, 32, 32}, fill_rec, {0.f, 0.f}, 0, WHITE);
    }
}

void App::renderCuttingBoardMenu(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Menu background
    Rectangle menu_rec = Rectangle{10.f, 10.f, screen_w - 20.f, screen_h - 20.f};
    DrawRectangleRec(menu_rec, ColorAlpha(WHITE, 0.7));

    // Shop Title
    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10 + 1, menu_rec.y + 10 + 1, menu_rec.width - 20.f, 30}, "Cutting Board");
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
    GuiLabel(Rectangle{menu_rec.x + 10, menu_rec.y + 10, menu_rec.width - 20.f, 30}, "Cutting Board");

    // Exit Button
    setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 4, 30);
    if (GuiButton(Rectangle{menu_rec.x + 10, menu_rec.y + 10, 100.f, 35}, "Exit"))
        player.cooking_zone = plt::CookingZone_None;

    flecs::entity ing_e = ecs_world->get_alive(player.item);
    plt::Ingredient *ing_info = ing_e.get_mut<plt::Ingredient>();

    std::vector<std::string> cut_names = {"Left Cut", "Right Cut", "Middle Cut"};

    // Draw Cutting Options
    for (int i = plt::LeftPile; i < plt::SingleKebab; i++)
    {
        // Rectangle where this ingredient will be drawn
        Rectangle fill_rec = {menu_rec.x + 10, menu_rec.y + 90 + 70 * (i - 1), 64, 64};

        setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_LEFT, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        GuiLabel({fill_rec.x + 80, fill_rec.y, 200, 64}, cut_names[i - 1].c_str());

        if (GuiButton(fill_rec, ""))
        {
            ing_info->state = (plt::IngredientState)i;
            player.cooking_zone = plt::CookingZone_None;
        }

        DrawTexturePro(meals_tex, {ing_info->pos.x, ing_info->pos.y + 32.f * i, 32, 32}, fill_rec, {0.f, 0.f}, 0, WHITE);
    }
}