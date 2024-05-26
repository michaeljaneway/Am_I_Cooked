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

    // Customer Values

    // Fonts
    //--------------------------------------------------------------------------------------
    Font lookout_font;
    //--------------------------------------------------------------------------------------

    // Textures
    //--------------------------------------------------------------------------------------
    // Player Texture
    Texture2D player_tex;

    // Food Texture
    Texture2D meals_tex;
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
    void initFood();

    void renderBagMenu(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderPlayerInventory(flecs::entity e, plt::Position &pos, plt::Player &player);

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