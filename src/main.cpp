#include <raylib.h>

#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>

#include "screen.h"
#include "buffer.h"
#include "config.h"
#include "default_config.h"



int main(int argc, char** argv)
{
    Config config = Config();

    Screen screen(&config);

    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE); 
    
    InitWindow(screen.width, screen.height, "Termanim");
    SetTargetFPS(config.target_fps);

    screen.font = LoadFontEx("assets/font.ttf", config.large_font_size, 0, 0);
    screen.font_width = MeasureTextEx(screen.font, "W", config.large_font_size, 0).x / config.large_font_size;
    
    vector<Buffer> buffers;
    buffers.emplace_back(&screen, nullptr, NONE);
     
    const char* default_argv[] = {NULL, /*"--help",*/ NULL};
    buffers[0].create_child("/bin/bash", default_argv);

    while (!WindowShouldClose())
    {
        screen.update();
         
        BeginDrawing();

        ClearBackground(config.colour_bg);
        
        buffers[0].handle_input();
        buffers[0].draw();
        
        // DrawFPS(20, 20);

        EndDrawing();
        
        // if (IsKeyPressed(KEY_R)) config = Config();

        if (screen.quit) CloseWindow();
    }
    CloseWindow();

    return 0;
}
