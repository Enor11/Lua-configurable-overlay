#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <lua.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "app_state.h"
#include "render.h"
#include "wayland_helper.h"
#include "lua_helper.h"

void print_help() {
  printf("\ttest for help\n");
}

enum commands {
  help,
  conf_file
};

enum commands command_exists (const char *inString) {
    if (strcmp(inString, "-h") == 0) return help;
    if (strcmp(inString, "-f") == 0) return conf_file;
    return -1;
}

int config_setup(struct wayland_state *state, int argc, char *argv[]) {
  state->COLOR_BACKGROUND = 0xFF1E1E2E;
  state->COLOR_TEXT = 0xFFCDD6F4;
  state->COLOR_SHADOW = 0xFF000000;
  state->row = 20;
  state->col = 10;
  state->scale = 2;
  state->config_file = "config.lua";

  if (argc != 1) {
    for (int i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
	printf("%d. ", i);
	switch (command_exists(argv[i])) {
	  case help:
	    printf("HELP COMMAND\n");
	    print_help();
	    break;
	  case conf_file:
	    printf("load config file COMMAND\n");
	    break;
	  default:
	    printf("No such command: %s, use -h for help\n", argv[i]);
	    break;
	}
	
      } else {
	printf("%s\n", argv[i]);
      }
    }
  }

  if (luaL_dofile(state->L, "config.lua") != LUA_OK) {
    fprintf(stderr, "Failed to load script: %s\n", lua_tostring(state->L, -1));
    lua_close(state->L);
    return 1;
  }

  call_lua_func(state->L, "init");

  state->width = state->col * 8 * state->scale + state->shadow_offsetX;
  state->height = state->row * 8 * state->scale + state->shadow_offsetY;

  state->content = malloc(state->row * sizeof(uint32_t *));
  for (int i = 0; i < state->row; i++) {
    state->content[i] = malloc((state->col + 1) * sizeof(uint32_t));
    for (int j = 0; j < state->col; j++)
      state->content[i][j] = ' ';

    state->content[i][state->col] = 0;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  struct wayland_state state = {
    .running = true,
  };
  state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  state.L = luaL_newstate();
  luaL_openlibs(state.L);

  setup_config_table(&state);

  if (config_setup(&state, argc, argv)) {
    return 1;
  }

  state.display = wl_display_connect(NULL);
  if (!state.display) {
    fprintf(stderr, "Failed to connect to Wayland display\n");
    return 1;
  }

  state.registry = wl_display_get_registry(state.display);
  wl_registry_add_listener(state.registry, &registry_listener, &state);
  
  wl_display_roundtrip(state.display);

  if (!state.compositor || !state.layer_shell || !state.output) {
    fprintf(stderr, "Required Wayland protocols not available\n");
    return 1;
  }

  wl_display_roundtrip(state.display);
  printf("Screen resolution discovered: %dx%d\n", state.screen_width, state.screen_height);
  printf("Window resolution: %dx%d\n", state.width, state.height);

  state.surface = wl_compositor_create_surface(state.compositor);
  
  state.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
    state.layer_shell, state.surface, NULL,
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "app_launcher");

  zwlr_layer_surface_v1_set_size(state.layer_surface, state.width, state.height);
  zwlr_layer_surface_v1_set_anchor(state.layer_surface,
    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
  zwlr_layer_surface_v1_set_margin(state.layer_surface, state.offset_y, 0, 0, state.offset_x);
  zwlr_layer_surface_v1_set_keyboard_interactivity(
    state.layer_surface,
    ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE
  );
  
  struct wl_region *empty_region = wl_compositor_create_region(state.compositor);
  wl_surface_set_input_region(state.surface, empty_region);
  wl_region_destroy(empty_region);
  
  zwlr_layer_surface_v1_add_listener(state.layer_surface, &layer_surface_listener, &state);

  wl_surface_commit(state.surface);
  wl_display_roundtrip(state.display);

  state.buffer = create_buffer(&state);
  state.offscreen_buffer = malloc(state.width * state.height * sizeof(uint32_t));
  wl_surface_attach(state.surface, state.buffer, 0, 0);

  printf("Wayland launcher started!\n");

  state.needs_redraw = true;

  while (state.running) {
    struct pollfd fds[1];
    fds[0].fd = wl_display_get_fd(state.display);
    fds[0].events = POLLIN;
    
    int ret = poll(fds, 1, 100);
    
    if (ret > 0) {
      if (wl_display_dispatch(state.display) == -1) {
        break;
      }
    } else if (ret == 0) {
      wl_display_dispatch_pending(state.display);
    } else {
      break;
    }

    trigger_update_event(state.L);

    if (state.needs_redraw) {
      draw_frame(&state);
      wl_display_flush(state.display);
      state.needs_redraw = false;
    }
  }

  // Cleanup
  if (state.buffer) wl_buffer_destroy(state.buffer);
  if (state.shm_data) munmap(state.shm_data, state.width * state.height * 4);
  if (state.layer_surface) zwlr_layer_surface_v1_destroy(state.layer_surface);
  if (state.surface) wl_surface_destroy(state.surface);
  if (state.layer_shell) zwlr_layer_shell_v1_destroy(state.layer_shell);
  if (state.compositor) wl_compositor_destroy(state.compositor);
  if (state.registry) wl_registry_destroy(state.registry);
  wl_display_disconnect(state.display);

  for (int i = 0; i < state.row; i++) {
    free(state.content[i]);
  }
  free(state.content);

  if (state.xkb_state) xkb_state_unref(state.xkb_state);
  if (state.xkb_keymap) xkb_keymap_unref(state.xkb_keymap);
  if (state.xkb_context) xkb_context_unref(state.xkb_context);

  lua_close(state.L);

  return 0;
}
