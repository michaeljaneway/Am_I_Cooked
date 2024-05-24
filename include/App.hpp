#pragma once
#include "main.hpp"

class App
{
private:
    int screen_w;
    int screen_h;

    std::unique_ptr<flecs::world> ecs_world;
    std::unique_ptr<Map> map;

    Font kaph_font;

    bool render_colliders;
    bool render_farmable;
    bool render_positions;

    // Textures
    //--------------------------------------------------------------------------------------

    // Tileset Texture
    Texture2D tileset_tex;

    // Plant Texture
    Texture2D plants_tex;
    Texture2D plants_grayscale_tex;

    // Player Forwards
    Texture2D player_fwd_tex;
    // Player Backwards
    Texture2D player_bck_tex;
    // Player Left
    Texture2D player_l_tex;
    // Player Right
    Texture2D player_r_tex;

    //--------------------------------------------------------------------------------------

    // Used to give style to inventory item
    plt::LoopingEase inv_rot;
    plt::LoopingEase inv_scale;
    plt::LoopingEase text_y_add;

    bool has_collected_bag;
    bool has_planted_seed;
    bool has_collected_plant;
    bool has_sold_plant;
    bool has_bought_seeds;

    // A matrix the size of the map to check if a farm tile is occupied
    std::vector<std::vector<flecs::entity_t>> occupied_plant_area;

    // Vector of sprites to be rendered, sorted by y-values
    std::vector<plt::SPR> render_orders;

    std::vector<plt::PlantMoneyInfo> plant_money_info;

    void initSystems();

    void PlayerSystem(flecs::entity e, plt::Position &pos, plt::Player &player);
    void PlantSystem(flecs::entity e, plt::Position &pos, plt::Plant &plant);
    void CollisionSystem(flecs::entity e, plt::Position &pos, plt::Collider &coll);
    void DynamicBodySystem(flecs::entity e, plt::Position &pos, plt::Collider &coll);
    void RenderSystem();

    // Render Util
    void drawAttentionArrow(Vector2 target);
    void drawTutorialText(std::string text);
    void drawPulseRect(Rectangle pulse_rec);
    void renderPlayerMoney(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderPlayerInventory(flecs::entity e, plt::Position &pos, plt::Player &player);
    void renderPlayerBuyMenu(flecs::entity e, plt::Position &pos, plt::Player &player);

public:
    App(int screen_w, int screen_h);
    ~App();

    void runFrame();
};