cmake_minimum_required(VERSION 3.25.1)
project(SpeedJam5)

# Generate compile_commands.json
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Set C++ (CXX) Standard to 2020
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

file(GLOB SOURCES "src/*.cpp" "src/*/*.cpp" "include/*.hpp" "include/*/*.hpp" "include/*.h" "include/*/*.h")

add_executable(${PROJECT_NAME} ${SOURCES})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Dependencies
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        master
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG        master
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG             master
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(raygui)

option(FLECS_STATIC "Build static flecs lib" OFF)
FetchContent_MakeAvailable(flecs)

target_include_directories(
    ${PROJECT_NAME} 
    PRIVATE 
    
    "${CMAKE_SOURCE_DIR}/include"
    "${raylib_SOURCE_DIR}/include"
    "${raygui_SOURCE_DIR}/src"
    "${flecs_SOURCE_DIR}/include"
)

target_link_libraries(
    ${PROJECT_NAME}
    
    raylib
    flecs
)

# Web Configurations
if (${PLATFORM} STREQUAL "Web")
    # Map assets to root of .data file
    set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/@/")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH --preload-file \"${ASSETS_DIR}\"")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif()