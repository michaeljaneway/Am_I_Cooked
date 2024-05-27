#pragma once
#include "main.hpp"

class App
{
private:
    int screen_w;
    int screen_h;

    // World Values
    std::unique_ptr<flecs::world> ecs_world;
    std::unique_ptr<Map> map;

    // Counter for speedrunning
    float time_counter;

    // Debug GUI Values
    bool render_colliders;
    bool render_positions;

    // Audio Values
    bool is_audio_initialized;

    // Gamestate Values
    plt::GameState game_state;
    plt::GameState prev_game_state;

    plt::Devil devil;

    // Customer Values
    std::vector<plt::Customer> customers;

    std::vector<std::string> Day1Dialogue;
    std::vector<std::string> Day2Dialogue;
    std::vector<std::string> Day3Dialogue;

    void addRandomCustomers(int count, int order_size);

    // Fonts
    //--------------------------------------------------------------------------------------
    Font lookout_font;
    Font fear_font;
    //--------------------------------------------------------------------------------------

    // Textures
    //--------------------------------------------------------------------------------------
    // Player Texture
    Texture2D player_tex;

    // Logo Texture
    Texture2D logo_tex;

    // Food Texture
    Texture2D meals_tex;

    // Customer Texture
    Texture2D customer_tex;
    
    // Devil Texture
    Texture2D devil_tex;
    //--------------------------------------------------------------------------------------

    // Audio
    //--------------------------------------------------------------------------------------
    std::vector<Music> game_music;

    void handleGameMusic();
    void playGameMusic(Music &mus);
    //--------------------------------------------------------------------------------------

    // Used to give style to inventory item
    plt::LoopingEase inv_rot;
    plt::LoopingEase inv_scale;
    plt::LoopingEase text_y_add;

    // Vector of sprites to be rendered, sorted by y-values
    std::vector<plt::SPR> render_orders;

    // List of available ingredients
    std::vector<plt::Ingredient> ingredients;
    std::vector<plt::Dish> dishes;
    std::vector<Vector2> bowl_fills;

    void initFood();

    void renderBagMenu(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderDishMenu(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderSinkMenu(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderCuttingBoardMenu(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderStoveMenu(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderPlayerInventory(flecs::entity e, plt::Position &pos, plt::Player &player);

    // Order Functions
    void addDishToOrder(plt::Order &o, plt::Dish dish);
    void addIngredientToOrder(plt::Order &o, plt::Ingredient ing);
    plt::Order getRandomOrder(int number_of_sides);

    void renderDevil(Rectangle target, Color color);
    void renderIngredient(plt::Ingredient &ing, Rectangle target, Color color);
    void renderDish(plt::Dish &dish, Rectangle target, Color color);

    void renderIngredientInstr(plt::Ingredient &ing, Vector2 pt, bool done);
    void renderDishInstr(plt::Dish &dish, Vector2 pt, bool done);
    void renderOrderInstr(plt::Order &order);

    bool areIngredientsEqual(plt::Ingredient &a, plt::Ingredient &b);
    bool areDishesEqual(plt::Dish &a, plt::Dish &b);

    bool isPlayerHoldingRightPiece(plt::Player &player, plt::Order &order);

    // ECS
    //--------------------------------------------------------------------------------------

    // Initialize systems and attatch them to the ECS world
    void initSystems();

    // Get input from player
    void PlayerSystem(flecs::entity e, plt::Position &pos, plt::Player &player);

    // Moves colliders based on position
    void CollisionSystem(flecs::entity e, plt::Position &pos, plt::Collider &coll);

    // Handle collisions for dynamic bodies
    void DynamicBodySystem(flecs::entity e, plt::Position &pos, plt::Collider &coll);

    // Handle Customers and Orders
    void CustomerSystem();

    // Render the world after all updates
    void RenderSystem();

    //--------------------------------------------------------------------------------------

    // Render Util
    void drawAttentionArrow(Vector2 target);
    void drawTutorialText(std::string text);
    void drawPulseRect(Rectangle pulse_rec);

public:
    App(int screen_w, int screen_h);
    ~App();

    void runFrame();
};