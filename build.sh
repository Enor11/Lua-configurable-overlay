#!/bin/bash
gcc -Wall main.c render.c wayland_helper.c lua_helper.c fonts/font8x8.c \
    xdg-shell-protocol.c wlr-layer-shell-protocol.c \
    -lwayland-client -lxkbcommon -llua -lm -ldl -o lua_overlay
