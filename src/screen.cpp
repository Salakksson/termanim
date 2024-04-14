#include "screen.h"


void Screen::update()
{
    this->width = GetScreenWidth();
    this->height = GetScreenHeight();
    this->aspect_ratio = static_cast<float>(this->width)/static_cast<float>(this->height);
}

Vector2 Screen::map(Vector2 coordinates)
{
    coordinates.x *= this->width;
    coordinates.y *= this->height;
    return coordinates;
}

Screen::Screen(Config* config)
{
    width = config->default_resolution_x;
    height = config->default_resolution_y;
    
    aspect_ratio  = static_cast<float>(this->width)/static_cast<float>(this->height);
    quit = false;
    this->config = config;
}


