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
}

void Buffer::draw()
{
    int line = 0;
    int column = 0;
    
    Vector2 location = this->screen->map(this->location);
    
    float font_height = this->fontsize;
    float font_width = this->screen->font_width * font_height;
    
    location.x += 2 * font_width;
    location.y += 2 * font_width;
    
    int column_max = this->screen->width / font_width - 2;
    int line_skips = 0;

    Font font = this->screen->font;

    Config* config = this->screen->config;
    
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
                line -= line_skips;
                break;
            default:
                if (isprint(c)) 
                {
                    char_pos = {location.x + font_width * column, location.y + font_height * line};
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
                    char_pos = {location.x + font_width * column, location.y + font_height * line};
                    DrawTextCodepoint(font, c, char_pos, font_height, config->colour_fg);
                    column++;
                }
                // else printf("Character %d/0x%02x is unprintable, the buffer may have read an incomplete file\n", c, c);
                break;
        }
    }
    char_pos.x += font_width;
    DrawRectangleV(char_pos, (Vector2){2, font_height}, config->colour_fg);
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
