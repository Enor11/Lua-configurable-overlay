#ifndef LUA_HELPER_H
#define LUA_HELPER_H

#include "app_state.h"

void setup_config_table(struct wayland_state *state);
void call_lua_func(lua_State *L, const char *func_name);
void trigger_key_event(lua_State *L, uint32_t key);
void trigger_update_event(lua_State *L);

#endif
