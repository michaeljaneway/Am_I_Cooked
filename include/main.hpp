#pragma once

// Compilation
//  - cd into build directory
//  - run 'emsdk activate'

// Configure:
//      emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release "-DCMAKE_EXE_LINKER_FLAGS=-s USE_GLFW=3" -DCMAKE_EXECUTABLE_SUFFIX=".html"

// Build:
//      cmake --build .

// Host (select the new HTML5 file):
//      python -m http.server 8888 --bind 0.0.0.0
//
//      Self:   http://localhost:8888/
//      Others: http://25.39.101.225:8888/

// Standard Library
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdio.h>
#include <map>
#include <filesystem>

// Graphics
#include "raylib.h"
#include "raymath.h"
#include "raygui.h"

// ECS
#include "flecs.h"

// Emscripten
#include <emscripten.h>

// Tiled loader
#include "cute_tiled.h"

// Collision
#include "cute_c2.hpp"

// Easing
#include "easing.h"

// Custom files
class Map;
class App;

#include "Components.hpp"
#include "Map.hpp"
#include "App.hpp"