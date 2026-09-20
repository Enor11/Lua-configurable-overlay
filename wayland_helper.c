#define _GNU_SOURCE
#include <stdio.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

#include "wayland_helper.h"
#include "lua_helper.h"

struct wl_buffer *create_buffer(struct wayland_state *state) {
  int stride = state->width * 4;
  int size = stride * state->height;

  int fd = memfd_create("buffer", 0);
  ftruncate(fd, size);

  state->shm_data = (uint32_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  struct wl_shm_pool *pool = wl_shm_create_pool(state->shm, fd, size);
  struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, state->width, state->height, stride, WL_SHM_FORMAT_ARGB8888);
  
  wl_shm_pool_destroy(pool);
  close(fd);

  return buffer;
}

static void output_handle_geometry(void *data, struct wl_output *wl_output,
                                 int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
                                 int32_t subpixel, const char *make, const char *model, int32_t transform) {}

static void output_handle_mode(void *data, struct wl_output *wl_output,
                               uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
  struct wayland_state *state = data;
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    state->screen_width = width;
    state->screen_height = height;
  }
}

static void output_handle_done(void *data, struct wl_output *wl_output) {}
static void output_handle_scale(void *data, struct wl_output *wl_output, int32_t factor) {}

const struct wl_output_listener output_listener = {
  .geometry = output_handle_geometry,
  .mode = output_handle_mode,
  .done = output_handle_done,
  .scale = output_handle_scale,
};

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface,
                                  uint32_t serial, uint32_t width, uint32_t height) {
  struct wayland_state *state = (struct wayland_state*)data;
  zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
  
  if (width != 0 && height != 0) {
    state->width = width;
    state->height = height;
    
    if (state->buffer) {
      wl_buffer_destroy(state->buffer);
    }
    state->buffer = create_buffer(state);
    wl_surface_attach(state->surface, state->buffer, 0, 0);
    wl_surface_commit(state->surface);
  }
}

static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface) {
  struct wayland_state *state = (struct wayland_state*)data;
  state->running = false;
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
  .configure = layer_surface_configure,
  .closed = layer_surface_closed,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
  .ping = xdg_wm_base_ping,
};

// Keyboard Listeners
static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
  struct wayland_state *state = data;
    
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    close(fd);
    return;
  }

  char *map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map_str == MAP_FAILED) {
    close(fd);
    return;
  }

  state->xkb_keymap = xkb_keymap_new_from_string(state->xkb_context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map_str, size);
  close(fd);

  if (state->xkb_state) {
    xkb_state_unref(state->xkb_state);
  }
  state->xkb_state = xkb_state_new(state->xkb_keymap);
}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
  struct wayland_state *state = data;
    
  if (state->xkb_state) {
    xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
  }
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t key_state) {
  struct wayland_state *state = data;

  if (key_state != WL_KEYBOARD_KEY_STATE_PRESSED || !state->xkb_state) {
    return;
  }
  // wayland keycodes are offset by 8 from XKB keycodes
  uint32_t xkb_keycode = key + 8;
  
  trigger_key_event(state->L, xkb_keycode);
  
  state->needs_redraw = true;
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {}
static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {}
static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay) {}

static const struct wl_keyboard_listener keyboard_listener = {
  .keymap = keyboard_keymap,
  .enter = keyboard_enter,
  .leave = keyboard_leave,
  .key = keyboard_key,
  .modifiers = keyboard_modifiers,
  .repeat_info = keyboard_repeat_info
};

// Seat Listeners
static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t caps) {
  struct wayland_state *state = data;
    
  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !state->keyboard) {
    state->keyboard = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
  }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {}

static const struct wl_seat_listener seat_listener = {
  .capabilities = seat_capabilities,
  .name = seat_name
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t id, const char *interface, uint32_t version) {
  struct wayland_state *state = (struct wayland_state*)data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor = (struct wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm = (struct wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    state->xdg_wm_base = (struct xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
  } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
    state->layer_shell = (struct zwlr_layer_shell_v1*)wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    if (!state->output) {
      state->output = (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 3);
      wl_output_add_listener(state->output, &output_listener, state);
    }
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    state->seat = (struct wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 7);
    wl_seat_add_listener(state->seat, &seat_listener, state);
  }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {}

const struct wl_registry_listener registry_listener = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};
