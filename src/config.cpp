#include "config.h"
#include "default_config.h"

string trim_whitespace(const string& str)
{
    string result;
    for (char c : str)
    {
        if (c == '#') return result;
        if (!std::isspace(c)) result += c;
    }
    return result;
}

Config::Config()
{
    char* home;
    assert (home = getenv("HOME"));
    
    string config_dir(home);
    config_dir.append("/.config/termanim");
    
    struct stat st;
    int result = stat(config_dir.c_str(), &st);
    perror("stat()");
    switch (result)
    {
        case -1:
            mkdir(config_dir.c_str(), 0755);
            printf("Created config directory '%s'\n", config_dir.c_str());
            break;
        case 0:
            break;
        default:
            printf("stat(config_dir.c_str(), &st) returned %d, might be fucky\n", result);
            exit(1);
            break;
    }
    
    config_dir.append("/settings.conf");
    int fd = open(config_dir.c_str(), O_RDWR | O_CREAT, 0755); 
    perror("open()");
    /* Known issue, if root user has the same home directory as a user,
     * that user will not be permitted to modify the file, 
     * can change flags to fix this but may allow other a user to modify another users config */
    
    if (fd == -1)
    {
        printf("Failed to open config file '%s'\n", config_dir.c_str());
        exit(1);
    }
    
    if (fstat(fd, &st) < 0)
    {
        printf("Failed to read fstat of '%s'\n", config_dir.c_str());
        exit(1);
    }
   
    char* config_file; // Because c++ is goofy i have to initialise this before the goto statement

    if (st.st_size < 1)
    {
        write(fd, default_config, sizeof(default_config)-1);
        goto skip_reading;
    }
    config_file = (char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    perror("mmap()");
    if (config_file == MAP_FAILED)
    {
        printf("Failed to read config file '%s'\n", config_dir.c_str());
        exit(1);
    }
    
    this->parse(config_file);

    munmap(config_file, st.st_size);
    
    skip_reading:

    this->conf_fd = fd;

    int inotify_fd = inotify_init();
    perror("inotify_init()");
    if (inotify_fd == -1)
    {
        printf("Failed to initialise inotify, wont crash the app but if you see this configs wont hot-reload\n");
        return;
    }

    int watch_descriptor = inotify_add_watch(inotify_fd, config_dir.c_str(), IN_MODIFY);
    perror("inotify_add_watch()");
    if (watch_descriptor == -1)
    {
        printf("Failed to watch file '%s'\n", config_dir.c_str());
        return;
    }

}


void Config::parse(char* config_file)
{

    std::unordered_map<string, std::function<void(string)>> handlers = 
    {
        {"window_x", [this](string value) { default_resolution_x = std::stoi(value); }},
        {"window_y", [this](string value) { default_resolution_y = std::stoi(value); }},
        {"pane_border", [this](string value) { pane_border = std::stoi(value); }},
        {"target_fps", [this](string value) { target_fps = std::stoi(value); }},
        {"large_font_size", [this](string value) { large_font_size = std::stoi(value); }},
        {"tabsize", [this](string value) { tabsize = std::stoi(value); }},
        {"fontsize", [this](string value) { fontsize = std::stoi(value); }},
        {"colour_fg", [this](string value) { colour_fg = GetColor(0xff + 0x100 * std::stoi(value, nullptr, 16)); }},
        {"colour_bg", [this](string value) { colour_bg = GetColor(0xff + 0x100 * std::stoi(value, nullptr, 16)); }},
    };

    string str(config_file);

    istringstream iss(str);     
    
    string line;

    int line_number = 1;
    
    while (std::getline(iss, line))
    {
        string line_trimmed = trim_whitespace(line);

        if (line_trimmed.empty()) 
        {
            line_number++;
            continue;
        }

        size_t equal_pos = line_trimmed.find('=');
        
        if (equal_pos == std::string::npos)
        {
            printf("Error: parsing config on line:\n%d: %s\n", line_number, line.c_str());
            line_number++;
            continue;
        }

        std::string key = line_trimmed.substr(0, equal_pos);
        std::string value = line_trimmed.substr(equal_pos + 1);

        auto it = handlers.find(key);
        if (it != handlers.end())
        {
            it->second(value);
        }
        else 
        {
            printf("Error: unknown key '%s' on line:\n%d: %s\n", key.c_str(), line_number, line.c_str());
        }


        line_number++;
        
    }
}
