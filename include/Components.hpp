#pragma once
#include "main.hpp"

namespace plt
{
    //--------------------------------------------------------------------------------------
    // Cooking Zones
    //--------------------------------------------------------------------------------------

    enum CookingZoneType
    {
        CookingZone_None,
        CookingZone_Bag,
        CookingZone_Dishes,
        CookingZone_Sink,
        CookingZone_CuttingBoard,
        CookingZone_Stove,
        CookingZone_Trash
    };

    struct CookingZone
    {
        CookingZoneType type;
        Rectangle zone;
    };

    //--------------------------------------------------------------------------------------
    // Food Order
    //--------------------------------------------------------------------------------------

    enum DishType
    {
        Plate,
        Bowl,
        PlateAndKebab
    };

    enum IngredientState
    {
        Whole, // RAW
        LeftPile,
        CenterPile,
        RightPile,
        SingleKebab,
        ThreeKebabs,
        TopKebab,
        MiddleKebab,
        BottomKebab
    };

    struct Ingredient
    {
        std::string name;
        Vector2 pos;
        bool cookable;
        IngredientState state;
    };

    //--------------------------------------------------------------------------------------
    // Entity position and rotation (in degrees)
    //--------------------------------------------------------------------------------------
    struct Position
    {
        float x, y, rotation;
    };

    //--------------------------------------------------------------------------------------
    // Entity position and rotation (in degrees)
    //--------------------------------------------------------------------------------------
    struct Collider
    {
        // x and y are the respective offsets, then w and h
        Rectangle bounds;
        c2AABB body;
    };

    struct SolidBody
    {
        int i;
    };

    struct DynamicBody
    {
        int i;
    };

    //--------------------------------------------------------------------------------------
    // Player
    //--------------------------------------------------------------------------------------
    enum PlayerMvnmtState : uint8_t
    {
        PlayerMvnmtState_Left,
        PlayerMvnmtState_Right,
        PlayerMvnmtState_Back,
        PlayerMvnmtState_Forward
    };

    struct Player
    {
        bool on_farmable_land;
        PlayerMvnmtState move_state;

        // Last Frame Change
        float last_fchange;

        // Set time per frame change
        float time_per_fchange;

        // Time till next frame change
        float time_till_fchange;

        uint8_t current_frame;

        // Item the player is holding
        flecs::entity_t item;

        // Is the player holding an item
        bool is_holding_item;

        // Interacting with cooking zone
        CookingZoneType cooking_zone;
    };

    //--------------------------------------------------------------------------------------
    // Game State
    //--------------------------------------------------------------------------------------

    enum GameState
    {
        GameState_MainMenu,
        GameState_Day1Intro,
        GameState_Day1,
        GameState_Day2Intro,
        GameState_Day2,
        GameState_Day3Intro,
        GameState_Day3,
        GameState_Outro,
    };

    enum GameMusic : uint8_t
    {
        GameMusic_MainMenu,
        GameMusic_Day1,
        GameMusic_Day2,
        GameMusic_Day3,
        GameMusic_Devil,
        GameMusic_Ascension,
    };

    //--------------------------------------------------------------------------------------
    // Sprite Render Order (or Instruction) (for y-level rendering)
    //--------------------------------------------------------------------------------------
    struct SPR
    {
        float y_level;

        Texture2D tex;
        Rectangle source;
        Vector2 position;
        Color color;
    };

    //--------------------------------------------------------------------------------------
    // Sprite Render Order (or Instruction) (for y-level rendering)
    //--------------------------------------------------------------------------------------
    struct LoopingEase
    {
        float val;

        float set_time;
        float cur_time_left;

        bool increasing;

        float min_val;
        float max_val;

        easing_functions func;
    };
}