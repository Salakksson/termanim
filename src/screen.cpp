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

Rectangle Screen::map(Vector2 position, Vector2 size)
{
    Vector2 new_pos = this->map(position);
    Vector2 new_size = this->map(size);
    return (Rectangle){new_pos.x, new_pos.y, new_size.x, new_size.y};
}

Screen::Screen(Config* config)
{
    width = config->default_resolution_x;
    height = config->default_resolution_y;
    
    aspect_ratio  = static_cast<float>(this->width)/static_cast<float>(this->height);
    quit = false;
    this->config = config;
}


