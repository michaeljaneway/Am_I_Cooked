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

int pointInAABB(c2AABB A, c2v B)
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
void App::addDishToOrder(plt::Order &o, plt::Dish dish)
{
    o.dishes.push_back(dish);
    o.indicies.push_back(std::make_pair(0, o.dishes.size() - 1));
}

void App::addIngredientToOrder(plt::Order &o, plt::Ingredient ing)
{
    o.ingredients.push_back(ing);
    o.indicies.push_back(std::make_pair(1, o.ingredients.size() - 1));
}

plt::Order App::getRandomOrder(int number_of_sides)
{
    plt::Order f_order;
    f_order.completion = 0;

    std::random_device rd;  // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator

    std::uniform_int_distribution<> dishtype_dist(0, 1);
    std::uniform_int_distribution<> filltype_dist(0, 4);

    plt::DishType dish_type = (plt::DishType)(dishtype_dist(gen));

    plt::Dish dish;
    switch (dish_type)
    {
    case plt::DishType_Plate:
        dish = dishes[1];
        addDishToOrder(f_order, dish);
        break;

    case plt::DishType_Bowl:
        dish = dishes[0];
        dish.fill = (plt::BowlFillType)(filltype_dist(gen));
        addDishToOrder(f_order, dish);
        break;

    default:
        break;
    }

    for (int i = 0; i < number_of_sides; i++)
    {
        plt::Ingredient new_ingredient = ingredients[rand() % ingredients.size()];
        new_ingredient.state = (plt::IngredientState)(rand() % 3 + 1);
        addIngredientToOrder(f_order, new_ingredient);
    }

    return f_order;
}

void App::renderIngredient(plt::Ingredient &ing, Rectangle target, Color color)
{
    DrawTexturePro(meals_tex, {ing.pos.x, ing.pos.y + 32.f * ing.state, 32, 32}, target, {0.f, 0.f}, 0, color);
}

void App::renderDevil(Rectangle target, Color color)
{
    DrawTexturePro(devil_tex, {64.f * (devil.frame % 3), 64.f * (devil.frame / 3), 64, 64}, target, {0.f, 0.f}, 0, color);
}

void App::renderDish(plt::Dish &dish, Rectangle target, Color color)
{
    DrawTexturePro(meals_tex, {dish.pos.x, dish.pos.y, 32, 32}, target, {0.f, 0.f}, 0, WHITE);

    // Draw Fill
    if (dish.fill != plt::BowlFillType_None)
    {
        int fill_int = ((int)dish.fill) - 1;
        DrawTexturePro(meals_tex, {bowl_fills[fill_int].x, bowl_fills[fill_int].y, 32, 32}, target, {0.f, 0.f}, 0, WHITE);
    }
}

void App::renderIngredientInstr(plt::Ingredient &ing, Vector2 pt, bool done)
{
    // Draw background rectangle
    Rectangle instr_area = {pt.x, pt.y, 192, 40};
    DrawRectangleRec(instr_area, RAYWHITE);
    DrawRectangleLinesEx(instr_area, 1, BLACK);

    // Draw sprite
    Rectangle sprite_area = {instr_area.x + 4, instr_area.y + 4, 32, 32};

    if (done)
        DrawRectangleRec(sprite_area, ColorAlpha(GREEN, 0.4));
    else
        DrawRectangleRec(sprite_area, ColorAlpha(BLUE, 0.4));

    renderIngredient(ing, sprite_area, WHITE);

    std::string desc_str = ing.name + "\n";

    switch (ing.state)
    {
    case plt::LeftPile:
        desc_str += "Left Cut";
        break;
    case plt::CenterPile:
        desc_str += "Middle Cut";
        break;
    case plt::RightPile:
        desc_str += "Right Cut";
        break;

    default:
        break;
    }

    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_LEFT, TEXT_ALIGN_MIDDLE, 23, 17);
    GuiLabel(Rectangle{sprite_area.x + 40, sprite_area.y + 5, 192 - 40, 40}, desc_str.c_str());
}

void App::renderDishInstr(plt::Dish &dish, Vector2 pt, bool done)
{
    // Draw background rectangle
    Rectangle instr_area = {pt.x, pt.y, 192, 40};
    DrawRectangleRec(instr_area, RAYWHITE);
    DrawRectangleLinesEx(instr_area, 1, BLACK);

    // Draw sprite
    Rectangle sprite_area = {instr_area.x + 4, instr_area.y + 4, 32, 32};

    if (done)
        DrawRectangleRec(sprite_area, ColorAlpha(GREEN, 0.4));
    else
        DrawRectangleRec(sprite_area, ColorAlpha(BLUE, 0.4));

    renderDish(dish, sprite_area, WHITE);

    std::string desc_str = dish.name + "\n";
    switch (dish.fill)
    {
    case plt::BowlFillType_None:
        desc_str += "No Fill";
        break;
    case plt::BowlFillType_Red:
        desc_str += "Red Fill";
        break;
    case plt::BowlFillType_Yellow:
        desc_str += "Yellow Fill";
        break;
    case plt::BowlFillType_Green:
        desc_str += "Green Fill";
        break;
    case plt::BowlFillType_Brown:
        desc_str += "Brown Fill";
        break;

    default:
        break;
    }
    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_LEFT, TEXT_ALIGN_MIDDLE, 23, 17);
    GuiLabel(Rectangle{sprite_area.x + 40, sprite_area.y + 5, 192 - 40, 40}, desc_str.c_str());
}

void App::renderOrderInstr(plt::Order &order)
{
    for (int part = 0; part < order.indicies.size(); part++)
    {
        // Dish
        if (order.indicies[part].first == 0)
            renderDishInstr(order.dishes[order.indicies[part].second], {432.f, part * 40.f}, part < order.completion);
        else
            renderIngredientInstr(order.ingredients[order.indicies[part].second], {432.f, part * 40.f}, part < order.completion);
    }
}

bool App::areIngredientsEqual(plt::Ingredient &a, plt::Ingredient &b)
{
    return a.name == b.name && a.state == b.state;
}

bool App::areDishesEqual(plt::Dish &a, plt::Dish &b)
{
    return a.name == b.name && a.type == b.type && a.fill == b.fill;
}

void App::addRandomCustomers(int count, int order_size)
{
    for (int i = 0; i < count; i++)
    {
        plt::Customer new_customer;
        new_customer.type = (plt::CustomerType)(rand() % (plt::CustomerType_Built + 1));
        new_customer.state = plt::CustomerState_InLine;
        new_customer.col = Color{(unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(204)};
        new_customer.order = getRandomOrder(order_size);
        new_customer.pos = {6 * 32, 12 * 32};
        customers.push_back(new_customer);
    }
}

bool App::isPlayerHoldingRightPiece(plt::Player &player, plt::Order &order)
{
    if (player.holding_type == plt::PlayerHoldingType_None)
        return false;

    // Get the item the player is currently holding
    flecs::entity player_item_e = ecs_world->get_alive(player.item);

    // Get the item required
    int order_needed_type = order.indicies[order.completion].first;
    int order_index = order.indicies[order.completion].second;

    switch (order_needed_type)
    {
    case 0: // Dishes
    {
        if (!player_item_e.has<plt::Dish>())
            return false;

        plt::Dish *player_dish = player_item_e.get_mut<plt::Dish>();
        return areDishesEqual(*player_dish, order.dishes[order_index]);
    }
    break;
    case 1: // Ingredients
    {
        if (!player_item_e.has<plt::Ingredient>())
            return false;

        plt::Ingredient *player_ing = player_item_e.get_mut<plt::Ingredient>();
        return areIngredientsEqual(*player_ing, order.ingredients[order_index]);
    }
    break;

    default:
        break;
    }

    return false;
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

    devil = {0, 9, 0.f, 0.1f};

    render_colliders = false;
    render_positions = false;

    is_audio_initialized = false;

    lookout_font = LoadFontEx("fonts/Lookout 7.ttf", 128, 0, 250);
    fear_font = LoadFontEx("fonts/Fear 11.ttf", 128, 0, 250);

    inv_rot = {0.0, 20, 20, true, -1, 1, EaseInOutCubic};
    inv_scale = {0.0, 10, 10, true, 0.1, 0.3, EaseInOutCubic};
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
    // Load the customer texture
    {
        Image cust_img = LoadImage("customers.png");
        customer_tex = LoadTextureFromImage(cust_img);
        UnloadImage(cust_img);
    }
    // Load the customer texture
    {
        Image logo_img = LoadImage("Am_I_cooked.png");
        logo_tex = LoadTextureFromImage(logo_img);
        UnloadImage(logo_img);
    }
    // Load the devil texture
    {
        Image devil_img = LoadImage("Fire 64x.png");
        devil_tex = LoadTextureFromImage(devil_img);
        UnloadImage(devil_img);
    }
    // Load the outro texture
    {
        Image outro_img = LoadImage("not_cooked.png");
        outro_tex = LoadTextureFromImage(outro_img);
        UnloadImage(outro_img);
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

    Day3Dialogue.push_back("ORGAN TIME!!");
    Day3Dialogue.push_back("HA HA HA HA...");
    Day3Dialogue.push_back("YOU'LL BE COOKIN' DOWN\nHERE FOREVER");
    Day3Dialogue.push_back("THATS WHY I'LL NEVER\nGIVE YOU THE CHANCE TO\nASCEND");
    Day3Dialogue.push_back("Unsurprisingly, my choice in chefs\nis impeccable once again");
    Day3Dialogue.push_back("Wow, you made it all the way here.");

    Day2Dialogue.push_back("I'm gonna crank up the orders,\nlet's see how you handle\nREAL HEAT");
    Day2Dialogue.push_back("NOT!!!");
    Day2Dialogue.push_back("Wow, that was fast");

    Day1Dialogue.push_back("Let's get cookin'");
    Day1Dialogue.push_back("Capice??");
    Day1Dialogue.push_back("Do this for three shifts and you go\nto HEAVEN");
    Day1Dialogue.push_back("Plate each item on the countertop\nbetween the two vases");
    Day1Dialogue.push_back("Throw out misclicks and failures\nin the trash bin");
    Day1Dialogue.push_back("Use the stove to prepare meat");
    Day1Dialogue.push_back("Use the cutting board to cut produce");
    Day1Dialogue.push_back("Use the sink to fill up bowl orders");
    Day1Dialogue.push_back("Plates and Bowls are in the cabinet");
    Day1Dialogue.push_back("All the ingredients are in the bag\non the far left");
    Day1Dialogue.push_back("You will fill out that recipe top\nto bottom IN ORDER");
    Day1Dialogue.push_back("Customers will come in and give\nyou recipies");
    Day1Dialogue.push_back("Let me give you the run down");
    Day1Dialogue.push_back("If you want to get out,\nyou're gonna have to cook");
    Day1Dialogue.push_back("I'm your most famous HOST:\nLucifer Morningstar");
    Day1Dialogue.push_back("Welcome to HELL");
    Day1Dialogue.push_back("Look who just fell down\n...press [SPACE] to continue...");
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
                                if (!pointInAABB(rectToAABB(c_zone.zone), c2v{pos.x, pos.y}))
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

    // Only go into the stove menu if:
    // Holding a Whole, cookable ingredient
    case plt::CookingZone_Stove:
        if (player.holding_type == plt::PlayerHoldingType_Ingredient)
        {
            flecs::entity ing_e = ecs_world->get_alive(player.item);
            plt::Ingredient *ing_info = ing_e.get_mut<plt::Ingredient>();

            if (ing_info->cookable && ing_info->state == plt::Whole)
            {
                player.cooking_zone = plt::CookingZone_Stove;
            }
        }
        break;

    // Plate a a dish/ingredient only if it the correct one
    case plt::CookingZone_Plating:
        if (isPlayerHoldingRightPiece(player, customers.back().order))
        {
            // Put item into the order
            customers.back().order.completion += 1;

            // Delete the item in your hand
            ecs_world->get_alive(player.item).destruct();
            player.holding_type = plt::PlayerHoldingType_None;
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
        switch (game_state)
        {
        case plt::GameState_Day1:
            addRandomCustomers(4, 2); // 4, 2
            break;
        case plt::GameState_Day2:
            addRandomCustomers(6, 3); // 6, 3
            break;
        case plt::GameState_Day3:
            addRandomCustomers(6, 5); // 6, 5
            break;
        default:
            break;
        }
    }
    prev_game_state = game_state;

    // We're done here if there's no customers
    if (customers.size() == 0)
    {
        switch (game_state)
        {
        case plt::GameState_Day1:
            game_state = plt::GameState_Day2Intro;
            break;
        case plt::GameState_Day2:
            game_state = plt::GameState_Day3Intro;
            break;
        case plt::GameState_Day3:
            game_state = plt::GameState_Outro;
            break;
        default:
            break;
        }
        return;
    }

    Rectangle pickup_point = {3 * 32, 6 * 32, 32, 32};
    Rectangle leave_point = {-1 * 32, 7 * 32, 32, 32};

    // Change state
    if (customers.back().order.completion == customers.back().order.indicies.size() && customers.back().state == plt::CustomerState_InLine)
    {
        customers.back().state = plt::CustomerState_GettingFood;
    }
    else if (customers.back().state == plt::CustomerState_GettingFood)
    {
        // Start leaving if food is grabbed
        if (pointInAABB(rectToAABB(pickup_point), c2v{customers.back().pos.x, customers.back().pos.y}))
        {
            customers.back().facing = plt::CustomerFacing_Left;
            customers.back().state = plt::CustomerState_Leaving;
        }
    }
    else if (customers.back().state == plt::CustomerState_Leaving)
    {
        // Remove Customer if they've left
        if (pointInAABB(rectToAABB(leave_point), c2v{customers.back().pos.x, customers.back().pos.y}))
            customers.pop_back();
    }

    int cust_size = customers.size();

    const float customer_speed = 1.3;

    for (int i = 0; i < cust_size; i++)
    {
        switch (customers[cust_size - 1 - i].state)
        {
        case plt::CustomerState_InLine: // Wait in line
        {
            Vector2 cust_pos = customers[cust_size - 1 - i].pos;
            Vector2 dest_pos = {6 * 32, 8.f * 32 + i * 15};

            Vector2 v = Vector2Subtract(dest_pos, cust_pos);

            if (std::abs(Vector2Length(v)) < 0.5)
                break;

            v = Vector2Normalize(v);
            v = Vector2Scale(v, 0.5);

            customers[cust_size - 1 - i].pos = Vector2Add(cust_pos, v);
        }
        break;

        case plt::CustomerState_GettingFood: // Go to pick up food
        {
            Vector2 cust_pos = customers[cust_size - 1 - i].pos;
            Vector2 dest_pos = {pickup_point.x + pickup_point.width / 2, pickup_point.y + pickup_point.height / 2};

            Vector2 v = Vector2Subtract(dest_pos, cust_pos);
            v = Vector2Normalize(v);
            v = Vector2Scale(v, customer_speed);

            customers[cust_size - 1 - i].pos = Vector2Add(cust_pos, v);
        }
        break;

        case plt::CustomerState_Leaving: // Leaving
        {
            Vector2 cust_pos = customers[cust_size - 1 - i].pos;
            Vector2 dest_pos = {leave_point.x + leave_point.width / 2, leave_point.y + leave_point.height / 2};

            Vector2 v = Vector2Subtract(dest_pos, cust_pos);
            v = Vector2Normalize(v);
            v = Vector2Scale(v, customer_speed);

            customers[cust_size - 1 - i].pos = Vector2Add(cust_pos, v);
        }
        break;

        default:
            break;
        }
    }
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
        SetMusicVolume(game_music.back(), 0.7);

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
        return;

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
    // Looping Easing
    processLoopingEase(inv_rot, ecs_world->delta_time());
    processLoopingEase(inv_scale, ecs_world->delta_time());
    processLoopingEase(text_y_add, ecs_world->delta_time());

    devil.last_switch += ecs_world->delta_time();
    if (devil.last_switch >= devil.time_to_switch)
    {
        devil.last_switch = 0;
        devil.frame = (devil.frame + 1) % devil.total_frames;
    }

    // Handle music
    handleGameMusic();

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Main menu
    if (game_state == plt::GameState_MainMenu)
    {
        ClearBackground(Color{0x2B, 0x26, 0x27, 0xFF});

        setGuiTextStyle(lookout_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        if (GuiButton(Rectangle{screen_w * 0.25f, 250, screen_w - (screen_w * 0.5f), 50}, "PLAY"))
            game_state = plt::GameState_Day1Intro;

        // DrawTextureRec(logo_tex, {0, 0, (float)logo_tex.width, (float)logo_tex.height}, {screen_w / 2 - (float)logo_tex.width / 2 + 1, 50 + 1}, BLACK);
        DrawTextureRec(logo_tex, {0, 0, (float)logo_tex.width, (float)logo_tex.height}, {screen_w / 2 - (float)logo_tex.width / 2, 50}, WHITE);

        EndDrawing();
        return;
    }
    else if (game_state == plt::GameState_Outro)
    {
        DrawTexture(outro_tex, 0, 0, WHITE);

        setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 80, 50);
        GuiLabel(Rectangle{0 + 2, 10 + 2, (float)screen_w, 250}, "You Have Ascended\nTo Heaven");
        setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 80, 50);
        GuiLabel(Rectangle{0, 10, (float)screen_w, 250}, "You Have Ascended\nTo Heaven");

        std::stringstream speedrun_stream;
        speedrun_stream << std::fixed << std::setprecision(2) << time_counter;

        setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 35, 30);
        GuiLabel({0 + 2, screen_h - 100.f + 2, (float)screen_w, 40}, ("Time: " + speedrun_stream.str()).c_str());
        setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 35, 30);
        GuiLabel({0, screen_h - 100.f, (float)screen_w, 40}, ("Time: " + speedrun_stream.str()).c_str());

        EndDrawing();
        return;
    }

    // Speedrun time counter
    time_counter += ecs_world->delta_time();

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
                      Vector2 draw_pos = {round(pos.x - 16), round(pos.y - 40)};

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

    flecs::filter<plt::CookingZone> zone_f = ecs_world->filter<plt::CookingZone>();
    zone_f.each([&](flecs::entity e, plt::CookingZone &zone)
                {
                    drawPulseRect(zone.zone); //
                });

    std::sort(render_orders.begin(), render_orders.end(), compSPR);
    for (auto &spr : render_orders)
    {
        DrawTextureRec(spr.tex, spr.source, spr.position, spr.color);
    }

    map->drawFront();

    if (customers.size() != 0)
    {
        // Draw parts of the order on the counter
        if (customers.back().state == plt::CustomerState_InLine || customers.back().state == plt::CustomerState_GettingFood)
        {
            Rectangle order_target_rectangle = {3 * 32, 5 * 32, 32, 32};
            int i = 0;
            for (auto &index : customers.back().order.indicies)
            {
                if (i >= customers.back().order.completion)
                    break;

                switch (index.first)
                {
                case 0: // Dishes
                    renderDish(customers.back().order.dishes[index.second], order_target_rectangle, WHITE);
                    break;

                case 1:
                    renderIngredient(customers.back().order.ingredients[index.second], order_target_rectangle, WHITE);
                    break;

                default:
                    break;
                }
                i++;
            }
        }

        // Draw Customers
        for (int i = customers.size() - 1; i >= 0; i--)
            DrawTextureRec(customer_tex, {customers[i].facing * 32.f, customers[i].type * 32.f, 32, 32}, customers[i].pos, customers[i].col);
    }
    //--------------------------------------------------------------------------------------
    // Render GUI
    //--------------------------------------------------------------------------------------

    if (
        game_state == plt::GameState_Day1 ||
        game_state == plt::GameState_Day2 ||
        game_state == plt::GameState_Day3)
    {
        flecs::filter<plt::Position, plt::Player> inv_f = ecs_world->filter<plt::Position, plt::Player>();
        inv_f.each([&](flecs::entity e, plt::Position &pos, plt::Player &player)
                   {
                       switch (player.cooking_zone)
                       {
                       case plt::CookingZone_None:
                           renderPlayerInventory(e, pos, player);
                           if (customers.size() > 0)
                               renderOrderInstr(customers.back().order);
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
                       case plt::CookingZone_Stove:
                           renderStoveMenu(e, pos, player);
                           break;

                       default:
                           break;
                       }
                       //
                   });
    }

    //--------------------------------------------------------------------------------------
    // Render Tutorial/ Devil conversations
    //--------------------------------------------------------------------------------------

    switch (game_state)
    {
    case plt::GameState_Day1Intro:
    {
        if (Day1Dialogue.size() == 0)
        {
            game_state = plt::GameState_Day1;
            break;
        }

        Rectangle speech_box_rect = Rectangle{10, screen_h - 110.f, screen_w - 20.f, 100};
        Rectangle speech_rect = Rectangle{speech_box_rect.x + 100, speech_box_rect.y, speech_box_rect.width - 100, 100};
        Rectangle devil_rect = Rectangle{speech_box_rect.x, speech_box_rect.y, speech_box_rect.width - speech_rect.width, 100};

        DrawRectangleRec(speech_box_rect, ColorAlpha(BLACK, 0.9));
        renderDevil(devil_rect, WHITE);

        setGuiTextStyle(lookout_font, ColorToInt(WHITE), TEXT_ALIGN_LEFT, TEXT_ALIGN_TOP, 40, 30);
        GuiLabel(speech_rect, Day1Dialogue.back().c_str());

        if (IsKeyPressed(KEY_SPACE))
            Day1Dialogue.pop_back();
    }
    break;

    case plt::GameState_Day2Intro:
    {
        if (Day2Dialogue.size() == 0)
        {
            game_state = plt::GameState_Day2;
            break;
        }

        Rectangle speech_box_rect = Rectangle{10, screen_h - 110.f, screen_w - 20.f, 100};
        Rectangle speech_rect = Rectangle{speech_box_rect.x + 100, speech_box_rect.y, speech_box_rect.width - 100, 100};
        Rectangle devil_rect = Rectangle{speech_box_rect.x, speech_box_rect.y, speech_box_rect.width - speech_rect.width, 100};

        DrawRectangleRec(speech_box_rect, ColorAlpha(BLACK, 0.9));
        renderDevil(devil_rect, WHITE);

        setGuiTextStyle(lookout_font, ColorToInt(WHITE), TEXT_ALIGN_LEFT, TEXT_ALIGN_TOP, 40, 30);
        GuiLabel(speech_rect, Day2Dialogue.back().c_str());

        if (IsKeyPressed(KEY_SPACE))
            Day2Dialogue.pop_back();
    }
    break;

    case plt::GameState_Day3Intro:
    {
        if (Day3Dialogue.size() == 0)
        {
            game_state = plt::GameState_Day3;
            break;
        }

        Rectangle speech_box_rect = Rectangle{10, screen_h - 110.f, screen_w - 20.f, 100};
        Rectangle speech_rect = Rectangle{speech_box_rect.x + 100, speech_box_rect.y, speech_box_rect.width - 100, 100};
        Rectangle devil_rect = Rectangle{speech_box_rect.x, speech_box_rect.y, speech_box_rect.width - speech_rect.width, 100};

        DrawRectangleRec(speech_box_rect, ColorAlpha(BLACK, 0.9));
        renderDevil(devil_rect, WHITE);

        setGuiTextStyle(lookout_font, ColorToInt(WHITE), TEXT_ALIGN_LEFT, TEXT_ALIGN_TOP, 40, 30);
        GuiLabel(speech_rect, Day3Dialogue.back().c_str());

        if (Day3Dialogue.size() == 1)
            renderDevil({496.f, 64.f, 64.f, 64.f}, WHITE);

        if (IsKeyPressed(KEY_SPACE))
            Day3Dialogue.pop_back();
    }
    break;

    default:
        break;
    }

    //--------------------------------------------------------------------------------------
    // Render Speedrunning timer
    //--------------------------------------------------------------------------------------

    std::stringstream speedrun_stream;
    speedrun_stream << std::fixed << std::setprecision(2) << time_counter;

    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_LEFT, TEXT_ALIGN_BOTTOM, 28, 30);
    GuiLabel({10 + 1, screen_h - 40.f + 1, 200, 40}, speedrun_stream.str().c_str());
    setGuiTextStyle(lookout_font, ColorToInt(WHITE), TEXT_ALIGN_LEFT, TEXT_ALIGN_BOTTOM, 28, 30);
    GuiLabel({10, screen_h - 40.f, 200, 40}, speedrun_stream.str().c_str());

    //--------------------------------------------------------------------------------------
    // DEBUG RENDER SETTINGS
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
    DrawRectangleLinesEx(pulse_rec, 1, ColorAlpha(RED, inv_scale.val));
}

void App::renderPlayerInventory(flecs::entity e, plt::Position &pos, plt::Player &player)
{
    // Draw Background Rectangle
    Rectangle inv_rect = Rectangle{screen_w - 110.f, screen_h - 110.f, 100, 100};
    DrawRectangleRec(inv_rect, ColorAlpha(WHITE, 0.4));

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
        renderIngredient(*(item.get_mut<plt::Ingredient>()), target_rec, WHITE);
        break;

    // Render a dish item
    case plt::PlayerHoldingType_Dish:
        renderDish(*(item.get_mut<plt::Dish>()), target_rec, WHITE);
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
        Rectangle ing_rec = {menu_rec.x + 10 + 66 * (i % 9), menu_rec.y + 80 + 66 * (i / 9), 64, 64};

        if (GuiButton(ing_rec, ""))
        {
            flecs::entity ing_e = ecs_world->entity();
            ing_e.set<plt::Ingredient>(ing);

            player.holding_type = plt::PlayerHoldingType_Ingredient;
            player.item = ing_e.id();
            player.cooking_zone = plt::CookingZone_None;
        }

        DrawTexturePro(meals_tex, {ing.pos.x, ing.pos.y, 32, 32}, ing_rec, {0.f, 0.f}, 0, WHITE);

        Vector2 mouse_pos = GetMousePosition();
        if (pointInAABB(rectToAABB(ing_rec), c2v{mouse_pos.x, mouse_pos.y}))
        {
            setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 23, 17);
            GuiLabel({menu_rec.x + 10 + 1, menu_rec.y + 45 + 1, menu_rec.width - 20.f, 30}, ing.name.c_str());

            setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 23, 17);
            GuiLabel({menu_rec.x + 10, menu_rec.y + 45, menu_rec.width - 20.f, 30}, ing.name.c_str());
        }

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
        Rectangle dish_rec = {menu_rec.x + 25 + 276 * i, menu_rec.y + 70, 256, 256};

        if (GuiButton(dish_rec, ""))
        {
            flecs::entity dish_e = ecs_world->entity();
            dish_e.set<plt::Dish>(dish);

            player.holding_type = plt::PlayerHoldingType_Dish;
            player.item = dish_e.id();
            player.cooking_zone = plt::CookingZone_None;
        }

        setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 23, 17);
        GuiLabel({dish_rec.x + 1, dish_rec.y + dish_rec.height + 1, dish_rec.width, 40}, dish.name.c_str());
        setGuiTextStyle(lookout_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 23, 17);
        GuiLabel({dish_rec.x, dish_rec.y + dish_rec.height, dish_rec.width, 40}, dish.name.c_str());

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

void App::renderStoveMenu(flecs::entity e, plt::Position &pos, plt::Player &player)
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