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
    // SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE); 
    
    InitWindow(screen.width, screen.height, "Termanim");
    SetTargetFPS(config.target_fps);

    screen.font = LoadFontEx("assets/font.ttf", config.large_font_size, 0, 0);
    screen.font_width = MeasureTextEx(screen.font, "W", config.large_font_size, 0).x / config.large_font_size;
    
    vector<Buffer> buffers;
    buffers.emplace_back(&screen, nullptr, NONE);
    
    const char* default_argv[] = {NULL, NULL};
    buffers[0].create_child("/bin/bash", default_argv);
    
    
    Buffer* active_buffer = &buffers[0];
    while (!WindowShouldClose())
    {
        screen.update();
         
        BeginDrawing();

        ClearBackground(config.colour_bg);
        
        for (Buffer &b : buffers)
        {
            if (&b == active_buffer) b.handle_input();
            b.draw_new();
        }
        // DrawFPS(20, 20);

        EndDrawing();
        
        if (IsKeyDown(KEY_LEFT_ALT))
        {
            if (IsKeyPressed(KEY_UP))
            {
                active_buffer = &buffers.emplace_back(&screen, active_buffer, UP);
                active_buffer->create_child("/bin/bash", default_argv);
            }
            if (IsKeyPressed(KEY_DOWN))
            {
                active_buffer = &buffers.emplace_back(&screen, active_buffer, DOWN);
                active_buffer->create_child("/bin/bash", default_argv);
            }
            if (IsKeyPressed(KEY_LEFT))
            {
                active_buffer = &buffers.emplace_back(&screen, active_buffer, LEFT);
                active_buffer->create_child("/bin/bash", default_argv);
            }
            if (IsKeyPressed(KEY_RIGHT))
            {
                active_buffer = &buffers.emplace_back(&screen, active_buffer, RIGHT);
                active_buffer->create_child("/bin/bash", default_argv);
            }
        }

        if (screen.quit) CloseWindow();
    }
    CloseWindow();

    return 0;
}
