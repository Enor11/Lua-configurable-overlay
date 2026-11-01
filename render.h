#ifndef RENDER_H
#define RENDER_H

#include "app_state.h"

void draw_filled_rect(uint32_t *buffer, int buf_width, int buf_height, int x, int y, int w, int h, uint32_t color);
void draw_line(uint32_t *buffer, int buf_width, int buf_height, int x0, int y0, int x1, int y1, int thickness, uint32_t color);
void draw_char_glyph(uint32_t *buffer, int buf_width, int buf_height, const char *str, int x, int y, int scale, uint32_t color);
void draw_frame(struct wayland_state *state);

#endif
