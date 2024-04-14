#ifndef CONFIG
#define CONFIG

#include <raylib.h>

#include <assert.h>
#include <stdio.h>

#include <unordered_map>
#include <functional>
#include <sstream>
#include <string>
using std::istringstream;
using std::string;

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/inotify.h> 
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>


class Config
{
public:
    
    int default_resolution_x = 600;
    int default_resolution_y = 400;
    int target_fps = 244;

    int large_font_size = 150;
    
    int fontsize = 25;

    unsigned tabsize = 4;

    Color colour_bg = {26, 27, 38, 255};
    Color colour_fg = {165, 165, 165, 255}; 
    
    void parse(char* config_file);

    Config();


private:
    int conf_fd;
    int inotify_fd;
    int watch_descriptor;
};



#endif
