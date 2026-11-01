#include "lua_helper.h"
#include "app_state.h"
#include "render.h"
#include "wayland_helper.h"
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <string.h>

static int lua_draw_text(lua_State *L) {
	struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));
	const char *str = luaL_checkstring(L, 1);
	int x = (int)luaL_checkinteger(L, 2);
	int y = (int)luaL_checkinteger(L, 3);
	int scale = (int)luaL_checkinteger(L, 4);
	uint32_t color = (uint32_t)luaL_checkinteger(L, 5);

	draw_char_glyph(state->offscreen_buffer, state->width, state->height, str, x, y, scale, color);
	state->needs_redraw = true;
	return 0;
}

static int get_symbol(lua_State *L) {
	struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));

	uint32_t keycode = (int)luaL_checkinteger(L, 1);

	char pressed_key[8] = {0};
	int len = xkb_state_key_get_utf8(state->xkb_state, keycode, pressed_key, sizeof(pressed_key));

	if (len > 0) {
		lua_pushstring(L, pressed_key);
	}
	return 1;
}

static int fill_rect(lua_State *L) {
	struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));

	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	int w = (int)luaL_checkinteger(L, 3);
	int h = (int)luaL_checkinteger(L, 4);
	uint32_t color = (uint32_t)luaL_checkinteger(L, 5);

	draw_filled_rect(state->offscreen_buffer, state->width, state->height, x, y, w, h, color);
	state->needs_redraw = true;
	return 0;
}

static int line(lua_State *L) {
	struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));

	int x0 = (int)luaL_checkinteger(L, 1);
	int y0 = (int)luaL_checkinteger(L, 2);
	int x1 = (int)luaL_checkinteger(L, 3);
	int y1 = (int)luaL_checkinteger(L, 4);
	int thickness = (int)luaL_checkinteger(L, 5);
	uint32_t color = (uint32_t)luaL_checkinteger(L, 6);

	draw_line(state->offscreen_buffer, state->width, state->height, x0, y0, x1, y1, thickness, color);
	state->needs_redraw = true;
	return 0;
}

static int config_index(lua_State *L) {
	struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));
	const char *key = luaL_checkstring(L, 2);

	if (strcmp(key, "COLOR_BACKGROUND") == 0) lua_pushinteger(L, state->COLOR_BACKGROUND);
	else if (strcmp(key, "COLOR_SHAPE") == 0) lua_pushinteger(L, state->COLOR_SHAPE);
	else if (strcmp(key, "COLOR_SHADOW") == 0) lua_pushinteger(L, state->COLOR_SHADOW);
	else if (strcmp(key, "SHADOW_OFFSET_X") == 0) lua_pushinteger(L, state->shadow_offsetX);
	else if (strcmp(key, "SHADOW_OFFSET_Y") == 0) lua_pushinteger(L, state->shadow_offsetY);
	else if (strcmp(key, "WIDTH") == 0) lua_pushinteger(L, state->width);
	else if (strcmp(key, "HEIGHT") == 0) lua_pushinteger(L, state->height);
	else if (strcmp(key, "OFFSET_X") == 0) lua_pushinteger(L, state->offset_x);
	else if (strcmp(key, "OFFSET_Y") == 0) lua_pushinteger(L, state->offset_y);
	else if (strcmp(key, "RUNNING") == 0) lua_pushinteger(L, state->running);
	else if (strcmp(key, "FPS") == 0) lua_pushinteger(L, state->fps);
	else return 0;

	return 1;
}

static int config_newindex(lua_State *L) {
	struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));
	const char *key = luaL_checkstring(L, 2);

	if (strcmp(key, "COLOR_BACKGROUND") == 0) state->COLOR_BACKGROUND = (uint32_t)luaL_checkinteger(L, 3);
	else if (strcmp(key, "COLOR_SHAPE") == 0) state->COLOR_SHAPE = (uint32_t)luaL_checkinteger(L, 3);
	else if (strcmp(key, "COLOR_SHADOW") == 0) state->COLOR_SHADOW = (uint32_t)luaL_checkinteger(L, 3);
	else if (strcmp(key, "SHADOW_OFFSET_X") == 0) state->shadow_offsetX = (int)luaL_checkinteger(L, 3);
	else if (strcmp(key, "SHADOW_OFFSET_Y") == 0) state->shadow_offsetY = (int)luaL_checkinteger(L, 3);
	else if (strcmp(key, "WIDTH") == 0) request_surface_size(state, (int)luaL_checkinteger(L, 3), state->height);
	else if (strcmp(key, "HEIGHT") == 0) request_surface_size(state, state->width, (int)luaL_checkinteger(L, 3));
	else if (strcmp(key, "OFFSET_X") == 0) {
		state->offset_x = (int)luaL_checkinteger(L, 3);
		update_window_margins(state);
	}
	else if (strcmp(key, "OFFSET_Y") == 0) {
		state->offset_y = (int)luaL_checkinteger(L, 3);
		update_window_margins(state);
	}
	else if (strcmp(key, "RUNNING") == 0) state->running = (int)luaL_checkinteger(L, 3);
	else if (strcmp(key, "FPS") == 0) state->fps = (int)luaL_checkinteger(L, 3);

	state->needs_redraw = true;
	return 0;
}

void setup_config_table(struct wayland_state *state) {
	lua_newtable(state->L);
	lua_newtable(state->L);

	lua_pushlightuserdata(state->L, state);
	lua_pushcclosure(state->L, config_index, 1);
	lua_setfield(state->L, -2, "__index");

	lua_pushlightuserdata(state->L, state);
	lua_pushcclosure(state->L, config_newindex, 1);
	lua_setfield(state->L, -2, "__newindex");

	lua_setmetatable(state->L, -2);
	lua_setglobal(state->L, "config");

	lua_pushlightuserdata(state->L, state);
	lua_pushcclosure(state->L, fill_rect, 1);
	lua_setglobal(state->L, "fill_rect");

	lua_pushlightuserdata(state->L, state);
	lua_pushcclosure(state->L, line, 1);
	lua_setglobal(state->L, "draw_line");

	lua_pushlightuserdata(state->L, state);
	lua_pushcclosure(state->L, get_symbol, 1);
	lua_setglobal(state->L, "get_symbol");

	lua_pushlightuserdata(state->L, state);
	lua_pushcclosure(state->L, lua_draw_text, 1);
	lua_setglobal(state->L, "draw_text");
}

void call_lua_func(lua_State *L, const char *func_name) {
	lua_getglobal(L, func_name);
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			fprintf(stderr, "Lua Error (%s): %s\n", func_name, lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	} else {
		lua_pop(L, 1);
	}
}

void trigger_key_event(lua_State *L, uint32_t key) {
	lua_getglobal(L, "on_key");
	if (lua_isfunction(L, -1)) {
		lua_pushinteger(L, (lua_Integer)key);
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			fprintf(stderr, "Lua Error (on_key): %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	} else {
		lua_pop(L, 1);
	}
}

void trigger_update_event(lua_State *L) {
	lua_getglobal(L, "on_update");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			fprintf(stderr, "Lua Error (on_update): %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	} else {
		lua_pop(L, 1);
	}
}
