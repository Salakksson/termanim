#include "buffer.h"



Buffer::Buffer(Screen* screen, Buffer* current, Direction direction)
{
    this->screen = screen;

    this->pwd = string(getenv("HOME")).append("/");
    
    printf("pwd: %s\n", this->pwd.c_str());
    
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1)
    {
        perror("buffer.cpp - posix_openpt()");
        exit(1);
    }

    if (grantpt(master_fd) == -1)
    {
        perror("buffer.cpp - grantpt()");
        exit(1);
    }
    
    if (unlockpt(master_fd) == -1)
    {
        perror("buffer.cpp - unlockpt()");
        exit(1);
    }
    
    pty_name = ptsname(master_fd);
    if (!pty_name)
    {
        perror("buffer.cpp - ptsname()");
        exit(1);
    }

    if (!current)
	{
		location = {0, 0};
		size = {1, 1};
		fontsize = screen->config->fontsize;
		return;
	}
    else 
    {
        fontsize = current->fontsize;
        Vector2 oldloc = current->location;
        Vector2 oldsize = current->size;

        Vector2 size_horizontal = {oldsize.x/2, oldsize.y};
        Vector2 location_left = oldloc;
        Vector2 location_right = {oldloc.x + oldsize.x/2, oldloc.y};
        
        Vector2 size_vertical = {oldsize.x, oldsize.y/2};
        Vector2 location_top = oldloc;
        Vector2 location_bottom = {oldloc.x, oldloc.y + oldsize.y/2};
        
        switch(direction)
        {
            case UP:
                this->location = location_top;
                current->location = location_bottom; 
                this->size = size_vertical;
                current->size = size_vertical;
                break;
            case DOWN:
                this->location = location_bottom;
                current->location = location_top; 
                this->size = size_vertical;
                current->size = size_vertical;
                break;
            case LEFT:
                this->location = location_left;
                current->location = location_right; 
                this->size = size_horizontal;
                current->size = size_horizontal;
                break;
            case RIGHT:
                this->location = location_right;
                current->location = location_left; 
                this->size = size_horizontal;
                current->size = size_horizontal;
                break;
            default:
                printf("Unknown direction for tile split\n");
                break;
        }
    }
}

void Buffer::draw()
{
    static int total_lines = 0;
    
    int line = 0;
    int column = 0;
    
    float font_height = this->fontsize;
    float font_width = this->screen->font_width * font_height;
    
    int column_max = this->screen->width * this->size.x / font_width - 4;
    int line_max = this->screen->height * this->size.y / font_height - 3;
    int line_skips = 0;

    Font font = this->screen->font;

    Config* config = this->screen->config;
    
    Vector2 char_location = this->screen->map(this->location);
    
    char_location.x += 2 * font_width;
    char_location.y += 2 * font_width;
    
    if (total_lines > line_max) char_location.y -= font_height * (total_lines - line_max);

    Vector2 char_pos;
    for (char c : this->text)
    { 
        if (column > column_max)
        {
            column = 0;
            line++;
            line_skips++;
        }
        switch (c)
        {
            case '\n':
                line_skips = 0;
                line++; column = 0;
                break;
            case '\t':
                column = (column % 4) ? column + 4 - column % 4 : column + 4; 
                break;
            case '\r':
                column = 0;
                line -= line_skips - 1;
                break;
            default:
                if (isprint(c)) 
                {
                    char_pos = {char_location.x + font_width * column, char_location.y + font_height * line};
                    DrawTextCodepoint(font, c, char_pos, font_height, config->colour_fg);
                    column++;
                }
                else printf("Character %d/0x%02x is unprintable, the buffer may have read an incomplete file\n", c, c);
                break;
        }
    }

    for (char c : this->user_input)
    { 
        // Add check for when text is longer than terminal window
        switch (c)
        {
            case '\n':
                line++; column = 0;
                break;
            case '\t':
                column = (column % 4) ? column + 4 - column % 4 : column + 4; 
                break;
            default:
                if (isprint(c)) 
                {
                    char_pos = {char_location.x + font_width * column, char_location.y + font_height * line};
                    DrawTextCodepoint(font, c, char_pos, font_height, config->colour_fg);
                    column++;
                }
                break;
        } 
    }

    char_pos.x += font_width;
    // Draw pane boundaries
    DrawRectangleLinesEx(this->screen->map(this->location, this->size), config->pane_border, config->colour_fg);
    DrawRectangleV(char_pos, (Vector2){2, font_height}, config->colour_fg);
    

    total_lines = line;
}

void Buffer::create_child(const char* path, const char** argv)
{
    child_pid = fork();

    if (child_pid == 0)
    {
        
        int child_fd = open(pty_name, O_RDWR);
        if (child_fd == -1)
        {
            perror("open()");
        }
        if (dup2(child_fd, STDIN_FILENO) == -1)
        {
            perror("dup2()");
        }
        if (dup2(child_fd, STDOUT_FILENO) == -1)
        {
            perror("dup2()");
        }
        if (dup2(child_fd, STDERR_FILENO) == -1)
        {
            perror("dup2()");
        }
        
        close(child_fd);

        char *const envp[] = {NULL};
        argv[0] = path;
        
        setsid();
        execve(path, const_cast<char* const*>(argv), envp);
        
        perror("execve()");
        exit(1);
    }
    if (child_pid < 0)
    {
        perror("Cant fork process lmfao, heres the error if u want it");
        exit(0);
    } 

    printf("Created child process '%s' with pid %d\n", path, child_pid);
    
    pthread_t read_thread;
    Thread_args thread_args = {master_fd, child_pid, this};
    pthread_create(&read_thread, nullptr, thread_read, &thread_args);
    
}

void* thread_read(void* args)
{
    Thread_args* thread_args = static_cast<Thread_args*>(args);
    int child_fd = thread_args->child_fd;
    Buffer* buffer = thread_args->buffer;
    
    char read_buffer[1024];
    int bytes_read;
    
    do 
    {   
        bytes_read = read(child_fd, read_buffer, sizeof(read_buffer));
        if (bytes_read == -1)
        {
            memset(read_buffer, 0, sizeof(read_buffer));
            switch (errno)
            {
                case EBADF:
                    printf("File descriptor is invalid?\n");
                    bytes_read = -2;
                    break;
                case EIO:
                    perror("read()");
                    printf("Some fucked shit occured, hopefully the program wont segfault\n");
                    break;
                case EINTR:
                    printf("Retrying read\n");
                    break;
                default:
                    perror("read()");
                    printf("Some crazy fucked shit hapenned and imma close, goodbye\n");
                    exit(1);
                    
            }
            perror("read()");
            printf("Failed to read stdout of child, exiting\n");
            exit(1); 
        }
        else read_buffer[bytes_read] = 0;

        string filtered_buffer;
        for (int i = 0; i < bytes_read; i++)
        {
            if (read_buffer[i] != '\033')
            {
                filtered_buffer += read_buffer[i];
                continue;
            }
            for (i++; i < bytes_read && read_buffer[i] != 'm'; i++);
        }
        buffer->text += filtered_buffer;
    }
    while (bytes_read != -2); 
    
    printf("Process complete or failed\n");

    return nullptr;
}

void Buffer::handle_input()
{
    char c;
    while (c = GetCharPressed())
    {
        this->user_input.push_back(c);
    }
    if (IsKeyPressed(KEY_ENTER))
    {
        this->user_input.push_back('\n');
        write(this->master_fd, this->user_input.c_str(), this->user_input.length());
        this->user_input.clear();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && this->user_input.length() != 0)
        this->user_input.pop_back();
}
