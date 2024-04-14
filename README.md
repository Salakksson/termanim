# Termanim - Animated Terminal Emulator

Simple terminal emulator written in c++

## Features

- **Config file**: The terminal is configurable through the config file at ~/.config/termanim/settings.conf
- **Basic functionality**: The terminal is capable of running a shell and running binaries/scripts within it
- **Movements and bindings**: Im planning to implement movements and bindings to make navigating with a keyboard easy
- **Planes**: Im planning to implement multiple planes at the same time which can be swapped, this functionality is already possible but it requires bindings 

## Compilation

A makefile is used to compile the app

## Configuration

`settings.conf` follows a basic syntax like this:
```
# Comments
key = value
```
It is created the first time launching termanim and comes with a default config
The plan is to make the config be reloaded if it has been edited
## Known Issues

- **Escape sequences**: I havent implemented escape sequences, so text isnt coloured correctly (only one text colour, customisable using `colour_bg` in settings.conf)
- **TUI's dont work**: I havent implemented them yet, so apps such as vim will not work

## Contributing

If you want to contribute feel free to do so, however please follow the style used as to not make it inconsistent
