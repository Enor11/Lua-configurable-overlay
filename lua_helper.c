#include "lua_helper.h"
#include "app_state.h"
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <string.h>
#include "fonts/font8x8.h"

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

static int set_char(lua_State *L) {
  struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));

  int y = (int)luaL_checkinteger(L, 1);
  int x = (int)luaL_checkinteger(L, 2);
  
  if (x < 0 || x >= state->col || y < 0 || y >= state->row) {
    return 0;
  }

  const char* c = luaL_checkstring(L, 3);

  state->content[y][x] = utf8_decode(c);
  state->needs_redraw = true;
  return 0;
}

static int config_index(lua_State *L) {
  struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));
  const char *key = luaL_checkstring(L, 2);

  if (strcmp(key, "COLOR_BACKGROUND") == 0) lua_pushinteger(L, state->COLOR_BACKGROUND);
  else if (strcmp(key, "COLOR_TEXT") == 0) lua_pushinteger(L, state->COLOR_TEXT);
  else if (strcmp(key, "COLOR_SHADOW") == 0) lua_pushinteger(L, state->COLOR_SHADOW);
  else if (strcmp(key, "SHADOW_OFFSET_X") == 0) lua_pushinteger(L, state->shadow_offsetX);
  else if (strcmp(key, "SHADOW_OFFSET_Y") == 0) lua_pushinteger(L, state->shadow_offsetY);
  else if (strcmp(key, "ROWS") == 0) lua_pushinteger(L, state->row);
  else if (strcmp(key, "COLS") == 0) lua_pushinteger(L, state->col);
  else if (strcmp(key, "SCALE") == 0) lua_pushinteger(L, state->scale);
  else if (strcmp(key, "OFFSET_X") == 0) lua_pushinteger(L, state->offset_x);
  else if (strcmp(key, "OFFSET_Y") == 0) lua_pushinteger(L, state->offset_y);
  else if (strcmp(key, "RUNNING") == 0) lua_pushinteger(L, state->running);
  else return 0; // Return nil for unknown keys

  return 1;
}

static int config_newindex(lua_State *L) {
  struct wayland_state *state = (struct wayland_state *)lua_touserdata(L, lua_upvalueindex(1));
  const char *key = luaL_checkstring(L, 2);

  if (strcmp(key, "COLOR_BACKGROUND") == 0) state->COLOR_BACKGROUND = (uint32_t)luaL_checkinteger(L, 3);
  else if (strcmp(key, "COLOR_TEXT") == 0) state->COLOR_TEXT = (uint32_t)luaL_checkinteger(L, 3);
  else if (strcmp(key, "COLOR_SHADOW") == 0) state->COLOR_SHADOW = (uint32_t)luaL_checkinteger(L, 3);
  else if (strcmp(key, "SHADOW_OFFSET_X") == 0) state->shadow_offsetX = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "SHADOW_OFFSET_Y") == 0) state->shadow_offsetY = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "ROWS") == 0) state->row = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "COLS") == 0) state->col = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "SCALE") == 0) state->scale = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "OFFSET_X") == 0) state->offset_x = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "OFFSET_Y") == 0) state->offset_y = (int)luaL_checkinteger(L, 3);
  else if (strcmp(key, "RUNNING") == 0) state->running = (int)luaL_checkinteger(L, 3);

  state->needs_redraw = true;
  return 0;
}

void setup_config_table(struct wayland_state *state) {
  lua_newtable(state->L); // Main `config` table
  lua_newtable(state->L); // Metatable

  // Bind `config_index` with `state` pointer as upvalue
  lua_pushlightuserdata(state->L, state);
  lua_pushcclosure(state->L, config_index, 1);
  lua_setfield(state->L, -2, "__index");

  // Bind `config_newindex` with `state` pointer as upvalue
  lua_pushlightuserdata(state->L, state);
  lua_pushcclosure(state->L, config_newindex, 1);
  lua_setfield(state->L, -2, "__newindex");

  lua_setmetatable(state->L, -2);
  lua_setglobal(state->L, "config");

  // registers global functions
  lua_pushlightuserdata(state->L, state);
  lua_pushcclosure(state->L, set_char, 1);
  lua_setglobal(state->L, "set_char");

  lua_pushlightuserdata(state->L, state);
  lua_pushcclosure(state->L, get_symbol, 1);
  lua_setglobal(state->L, "get_symbol");
}

// Helper to safely call a Lua function with no arguments
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

// Pass a key press string event to Lua
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

