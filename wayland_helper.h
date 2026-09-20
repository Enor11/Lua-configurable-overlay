#ifndef WAYLAND_HELPER_H
#define WAYLAND_HELPER_H

#include "app_state.h"

struct wl_buffer *create_buffer(struct wayland_state *state);

extern const struct wl_output_listener output_listener;
extern const struct zwlr_layer_surface_v1_listener layer_surface_listener;
extern const struct xdg_wm_base_listener xdg_wm_base_listener;
extern const struct wl_registry_listener registry_listener;

#endif
