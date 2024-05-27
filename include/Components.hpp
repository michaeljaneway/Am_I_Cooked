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
        CookingZone_Trash,
        CookingZone_Plating
    };

    struct CookingZone
    {
        CookingZoneType type;
        Rectangle zone;
    };

    //--------------------------------------------------------------------------------------
    // Food Order
    //--------------------------------------------------------------------------------------

    enum IngredientState
    {
        Whole, // RAW
        LeftPile,
        RightPile,
        CenterPile,
        SingleKebab,
        ThreeKebabs,
        TopKebab,
        MiddleKebab,
        BottomKebab
    };

    enum BowlFillType
    {
        BowlFillType_None,
        BowlFillType_Red,
        BowlFillType_Yellow,
        BowlFillType_Green,
        BowlFillType_Brown,
    };

    enum DishType
    {
        DishType_Plate,
        DishType_Bowl
    };

    struct Dish
    {
        std::string name;
        DishType type;
        Vector2 pos;
        BowlFillType fill;
    };

    struct Ingredient
    {
        std::string name;
        Vector2 pos;
        bool cookable;
        IngredientState state;
    };

    // A restaurant order
    struct Order
    {
        std::vector<Dish> dishes;
        std::vector<Ingredient> ingredients;

        // Pair where:
        // - First: 0 for dishes, 1 for ingredients
        // - Second: Index in the respective list
        std::vector<std::pair<int, int>> indicies;

        int completion;
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

    struct Devil
    {
        int frame;
        int total_frames;
        float last_switch;
        float time_to_switch;
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

    enum PlayerHoldingType
    {
        PlayerHoldingType_None,
        PlayerHoldingType_Ingredient,
        PlayerHoldingType_Dish,
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

        // Type of item the player is holding
        PlayerHoldingType holding_type;

        // Interacting with cooking zone
        CookingZoneType cooking_zone;
    };

    //--------------------------------------------------------------------------------------
    // Customers
    //--------------------------------------------------------------------------------------

    enum CustomerType
    {
        CustomerType_Big,
        CustomerType_Thin,
        CustomerType_Built
    };

    enum CustomerFacing
    {
        CustomerFacing_Fwd,
        CustomerFacing_Left
    };

    enum CustomerState
    {
        CustomerState_InLine,
        CustomerState_GettingFood,
        CustomerState_Leaving,
    };

    struct Customer
    {
        CustomerType type;
        CustomerState state;
        CustomerFacing facing;
        Order order;
        Color col;

        Vector2 pos;
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