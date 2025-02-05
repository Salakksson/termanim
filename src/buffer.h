#ifndef BUFFER
#define BUFFER

#include <raylib.h>

#include "screen.h"

#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include <mutex>
#include <vector>
#include <tuple>
#include <string>

using std::vector;
using std::tuple;
using std::string;



enum Direction
{
	NONE,
	UP,
	DOWN,
	LEFT,
	RIGHT
};

class Buffer
{ 
public:
    
    Screen* screen;

	int page;

	Vector2 location;
	Vector2 size;
	
	float fontsize;
    
    vector<string> text_lines;
    string text = string("");
    string user_input = string("");
    
    void append_line(string& line);

    void draw_new();

    void draw();
    
    void handle_input();

    void create_child(const char* path, const char** argv);
    
    void read_child();

    Buffer(Screen* screen, Buffer* current, Direction direction);
     
private:
    
    pid_t child_pid;

    string pwd;

    vector<tuple<Buffer*, Direction>> borders;
    
    int master_fd;
    char* pty_name;
    
    float scroll_height;
    float scroll_height_add;

};



struct Thread_args
{
    int master_fd;
    int child_pid;
    Buffer* buffer;
};

void* thread_read(void* args);

#endif
