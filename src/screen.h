#ifndef SCREEN 
#define SCREEN

#include <raylib.h>

#include <stdio.h>

#include "config.h"

typedef struct Color Colour;


class Screen
{
public:

    Config* config;

    int width;
    int height;
    float aspect_ratio;
    
    Font font;
    float font_width;

    bool quit;

    void update();
    
    Vector2 map(Vector2 coordinates);

    Screen(Config* config);
};


#endif
