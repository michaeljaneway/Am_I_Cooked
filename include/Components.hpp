#pragma once
#include "main.hpp"

namespace plt
{
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
        PlayerMvnmtState_Idle,
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

        // The type of item the player is holding
        bool is_holding_item;

        // Is the player in the buy menu
        bool in_buy_menu;

        // Player's money
        int money;
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