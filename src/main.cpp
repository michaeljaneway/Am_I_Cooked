#include "main.hpp"

// Immediate-mode GUI Library
#define RAYGUI_IMPLEMENTATION
#define GLSL_VERSION 330
#include "raygui.h"

// Tiled loader
#define CUTE_TILED_IMPLEMENTATION
#include "cute_tiled.h"

// Collision
#define CUTE_C2_IMPLEMENTATION
#include "cute_c2.hpp"

std::unique_ptr<App> main_app;

void updateAndDraw()
{
    main_app->runFrame();
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screen_w = 20*32;
    const int screen_h = 12*32;

    // Set antialiasing
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    // Init window and framerate
    InitWindow(screen_w, screen_h, "SpeedJam5");

    // Set target FPS
    SetTargetFPS(60);

    // Initialize the main App
    main_app = std::make_unique<App>(screen_w, screen_h);

    // Set the emscripten main loop
    emscripten_set_main_loop(updateAndDraw, 0, 1);
    //-------------------------------------------------------------------------------------- 
 
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseAudioDevice();
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}
