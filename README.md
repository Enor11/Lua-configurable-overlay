# Lua-configurable-overlay
## About project
**Lua-configurable-overlay** is a Wayland overlay program thats made for `wlr-layer-shell` (Hyprland, Sway, Mangowm, etc.). 

This project was created to learn about C and Lua integration and to build a custom app launcher. The program can be fully customized via a Lua configuration file.

## Lua configuration
To configure using lua simply edit `config.lua`.

### configuration file
In `config.lua`, you should define at least these 3 functions:
```
-- Called when a key is pressed
function on_key(key)
end
-- Called each frame
function on_update()
end
-- Run at startup only once
function init()
end
```

### Configuration
#### Variables
To modify configuration for colors, offset and size change `config.[variable]` value.

These are the currently available options you can change:
```
-- Changes size of window
config.WIDTH = 900
config.HEIGHT = 600
-- Offset from top-left corner of screen
config.OFFSET_X = 10
config.OFFSET_Y = 10
-- Frame rate / how often on_update() is called
config.FPS = 20
```

#### Functions
```
-- Returns the symbol instead of the keycode (e.g., keycode 38 translates to symbol 'a')
get_symbol(key)
-- color should be in format 0xRRGGBBAA
fill_rect(x, y, width, height, color)
draw_text(text, x, y, size, color)
draw_line(x0, y0, x, y, width, color)
```

## How to build project
To build the project, you can simply run `build.sh`.

However, you need to have `curl` and `wayland-scanner` installed. To get required XDG and Wayland protocol files, run these commands:

```
# Download wlr-layer-shell and xdg-shell XML files
curl -O https://raw.githubusercontent.com/swaywm/wlroots/master/protocol/wlr-layer-shell-unstable-v1.xml
curl -O https://gitlab.freedesktop.org/wayland/wayland-protocols/-/raw/main/stable/xdg-shell/xdg-shell.xml

# Generate xdg-shell files
wayland-scanner client-header xdg-shell.xml xdg-shell-client-protocol.h
wayland-scanner private-code xdg-shell.xml xdg-shell-protocol.c

# Generate wlr-layer-shell files
wayland-scanner client-header wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-client-protocol.h
wayland-scanner private-code wlr-layer-shell-unstable-v1.xml wlr-layer-shell-protocol.c
```
