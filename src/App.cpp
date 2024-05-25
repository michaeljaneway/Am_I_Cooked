#include "App.hpp"

App::App(int screen_w, int screen_h)
{
    // ==================================================
    // Set screen w and h
    // ==================================================
    this->screen_w = screen_w;
    this->screen_h = screen_h;

    time_counter = 0;

    render_colliders = false;
    render_positions = false;

    // Load Font (https://ggbot.itch.io/kaph-font)
    // kaph_font = LoadFontEx("Kaph-Regular.ttf", 128, 0, 250);

    inv_rot = {0.0, 20, 20, true, -1, 1, EaseInOutCubic};
    inv_scale = {0.0, 40, 40, true, 0.95, 1, EaseInOutCubic};
    text_y_add = {0.0, 5, 5, true, 0, 10, EaseInOutCubic};

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
    // Load the tutorial textures
    // ==================================================

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
}

App::~App()
{
    UnloadTexture(player_fwd_tex);
    UnloadTexture(player_l_tex);
    UnloadTexture(player_r_tex);
    UnloadTexture(player_bck_tex);
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
    dist = Vector2Scale(dist, 4);
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
    // Switch actions based on what we're holding
    //--------------------------------------------------------------------------------------

    if (player.is_holding_item)
    {

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

    time_counter += ecs_world->delta_time();

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

    // GuiLabel(Rectangle{0, 0, 512, 200}, std::to_string(time_counter).c_str());

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
                      Vector2 draw_pos = {round(pos.x - 40), round(pos.y - 32 - 11)};

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

    //--------------------------------------------------------------------------------------
    // Render Tutorial
    //--------------------------------------------------------------------------------------


    //--------------------------------------------------------------------------------------
    //
    // DEBUG RENDER SETTINGS
    //
    //--------------------------------------------------------------------------------------

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiToggle(Rectangle{screen_w - 10.f - 100, 10, 100, 20}, "Render Colliders", &render_colliders);
    GuiToggle(Rectangle{screen_w - 10.f - 100, 40, 100, 20}, "Render Positions", &render_positions);

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