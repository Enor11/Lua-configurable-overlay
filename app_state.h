#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

struct wayland_state {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct xdg_wm_base *xdg_wm_base;
	struct zwlr_layer_shell_v1 *layer_shell;

	struct wl_output *output;
	int screen_width;
	int screen_height;

	struct wl_surface *surface;
	struct zwlr_layer_surface_v1 *layer_surface;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct wl_buffer *buffer;

	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	lua_State *L;
	char *config_file;

	uint32_t *shm_data;
	uint32_t *offscreen_buffer;
	int width, height;
	int offset_y, offset_x;

	// Colors (ARGB format)
	uint32_t COLOR_BACKGROUND;
	uint32_t COLOR_SHAPE;
	uint32_t COLOR_SHADOW;

	int shadow_offsetX;
	int shadow_offsetY;

	bool running;
	bool needs_redraw;
	int fps;
};

#endif
