#include "buffer.h"



Buffer::Buffer(Screen* screen, Buffer* current, Direction direction)
{
    this->screen = screen;

    this->pwd = string(getenv("HOME")).append("/");
    
    printf("pwd: %s\n", this->pwd.c_str());
    
    this->master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (this->master_fd == -1)
    {
        perror("buffer.cpp - posix_openpt()");
        exit(1);
    }

    if (grantpt(this->master_fd) == -1)
    {
        perror("buffer.cpp - grantpt()");
        exit(1);
    }
    
    if (unlockpt(this->master_fd) == -1)
    {
        perror("buffer.cpp - unlockpt()");
        exit(1);
    }
    
    pty_name = ptsname(this->master_fd);
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

void Buffer::append_line(string& line)
{
    this->text_lines.insert(this->text_lines.begin(), line);
    if (this->text_lines.size() > this->screen->config->max_lines)
        text_lines.pop_back();
}

void Buffer::draw_new()
{
    static int line_overflows;

    Font font = this->screen->font;
    Config* config = this->screen->config;
    Vector2 char_location = this->screen->map(this->location);
    
    float font_height = this->fontsize;
    float font_width = this->screen->font_width * font_height;
    
    int column_max = this->screen->width * this->size.x / font_width - 4;
    int line_max = this->screen->height * this->size.y / font_height - 3;

    int line = 0;
    int column = 0;
    
    char_location.x += 2 * font_width;
    char_location.y -= 2 * font_width;
    char_location.y += this->size.y * this->screen->height;

    Vector2 char_pos;

    for (string s : text_lines)
    {
        column = 0;
        line -= 1;
        for (char c : s)
        {
            char_pos = {char_location.x + font_width * column, char_location.y + font_height * line};
            DrawTextCodepoint(font, c, char_pos, font_height, config->colour_fg);
            column++;
        }
    }
    column = text_lines[0].length();
    line = 0;
    // if (text_lines.size() > 0) for (char c : string("poopy"))
    // {
    //     char_pos = {char_location.x + font_width * column, char_location.y + font_height * line};
    //     DrawTextCodepoint(font, c, char_pos, font_height, config->colour_fg);
    //     column++;
    // }
    DrawRectangleLinesEx(this->screen->map(this->location, this->size), config->pane_border, config->colour_fg);
    DrawRectangleV(char_pos, (Vector2){2, font_height}, config->colour_fg);
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
            write(child_fd, "failed to set child stdin", 10);
            exit(1);
        }
        if (dup2(child_fd, STDIN_FILENO) == -1)
        {
            perror("dup2()");
            write(child_fd, "failed to set child stdin", 10);
            exit(1);
        }
        if (dup2(child_fd, STDOUT_FILENO) == -1)
        {
            perror("dup2()");
            write(child_fd, "failed to set child stdout", 10);
            exit(1);
        }
        if (dup2(child_fd, STDERR_FILENO) == -1)
        {
            perror("dup2()");
            write(child_fd, "failed to set child stderr", 10);
            exit(1);
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
    Thread_args thread_args = {this->master_fd, child_pid, this};
    switch (pthread_create(&read_thread, nullptr, thread_read, &thread_args))
    {
        case 0:
            return;
        case EAGAIN:
            printf("pthread_create error EAGAIN: Insufficient resources to create thread or thread limit encountered\n");
            exit(1);
        case EINVAL:
            printf("pthread_create error EINVAL: this error should not occur, if it does then attributes of pthread_create must have been changed or some shit idk\n");
            exit(1);
        case EPERM:
            printf("pthread_create error EPERM: no permission or some shit idk\n");
            exit(1);
        default:
            printf("pthread_create error default: some mad fucked shit occured in pthread idk what this error si\n");
            exit(1); 
    }
    
}

void* thread_read(void* args)
{
    Thread_args* thread_args = static_cast<Thread_args*>(args);
    int master_fd = thread_args->master_fd;
    Buffer* buffer = thread_args->buffer;
    
    // std::mutex mutex;
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(master_fd, &readfds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    char read_buffer[1024];
    int bytes_read;
    int quit = 0; 
    printf("reading stdout:\n");
    while (!quit)
    {  
        // int select_result = select(master_fd + 1, &readfds, NULL, NULL, &timeout);
        // 
        // if (select_result < 0)
        // {
        //     perror("select()");
        //     exit(1);
        // }
        // else if (select_result >= 0)
        // {
        //     if (!FD_ISSET(master_fd, &readfds))
        //     {
        //         timeout.tv_sec = 1;
        //         timeout.tv_usec = 0;
        //         printf("timed out\n");
        //         continue;
        //     }
        //     else printf("reading file !!!!\n");
        // }

        printf("reading\n");
        bytes_read = read(master_fd, read_buffer, sizeof(read_buffer));
        if (bytes_read == -1)
        {
            memset(read_buffer, 0, sizeof(read_buffer));
            switch (errno)
            {
                case EBADF:
                    printf("File descriptor is invalid?\n");
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
        else if (bytes_read < 0) 
        {
            printf("shit fucked idk bytes_read = %d\n", bytes_read);
            exit(1);
        }
        read_buffer[bytes_read] = 0;
        
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
        // std::lock_guard<std::mutex> lock(mutex);
        buffer->text += filtered_buffer;
        istringstream iss(filtered_buffer);
        string line;
        while (std::getline(iss, line))
        {
            printf("line: %s\n", line.c_str());
            buffer->append_line(line);
        }
        if (bytes_read >= 0) printf("read: %s\n", read_buffer);
    }
    
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



