#include <string.h>
#include "render.h"
#include "fonts/font8x8.h"

void draw_char(uint32_t *buffer, int buf_width, int buf_height, int x, int y, uint32_t c, uint32_t color_background, uint32_t color_text, uint32_t color_shadow, int scale, int shadow_offsetX, int shadow_offsetY) {
  const uint8_t *glyph = get_glyph(c);

  for (int row = 0; row < 8; row++) {
    uint8_t line = glyph[row];
    for (int col = 0; col < 8; col++) {
      if (line & (1 << col)) {
        for (int sy = 0; sy < scale; sy++) {
          for (int sx = 0; sx < scale; sx++) {
            int px = x + col * scale + sx;
            int py = y + row * scale + sy;
            if (px >= 0 && px < buf_width && py >= 0 && py < buf_height) {
              buffer[py * buf_width + px] = color_text;
            }
          }
        }
        
        for (int sy = 0; sy < scale; sy++) {
          for (int sx = 0; sx < scale; sx++) {
            int px = x + col * scale + sx + shadow_offsetX;
            int py = y + row * scale + sy + shadow_offsetY;
            if (px >= 0 && px < buf_width && py >= 0 && py < buf_height) {
              if (buffer[py * buf_width + px] == color_background) {
                buffer[py * buf_width + px] = color_shadow;
              }
            }
          }
        }
      }
    }
  }
}

void draw_text(uint32_t *buffer, int buf_width, int buf_height,
               const uint32_t *text, int x, int y, uint32_t color_background, uint32_t color_text, uint32_t color_shadow, int scale, int shadow_offsetX, int shadow_offsetY) {
  for (int i = 0; text[i] != 0; i++) {
    draw_char(buffer, buf_width, buf_height, x + i * 8 * scale, y, text[i], color_background, color_text, color_shadow, scale, shadow_offsetX, shadow_offsetY);
  }
}

void draw_frame(struct wayland_state *state) {
  int pixel_count = state->width * state->height;

  for (int i = 0; i < pixel_count; i++) {
    state->offscreen_buffer[i] = state->COLOR_BACKGROUND;
  }

  for (int y = 0; y < state->row; y++) {
    draw_text(
      state->offscreen_buffer,
      state->width,
      state->height,
      state->content[y],
      0,
      y * 8 * state->scale,
      state->COLOR_BACKGROUND,
      state->COLOR_TEXT,
      state->COLOR_SHADOW,
      state->scale,
      state->shadow_offsetX,
      state->shadow_offsetY
    );
  }

  memcpy(state->shm_data, state->offscreen_buffer, pixel_count * sizeof(uint32_t));

  wl_surface_attach(state->surface, state->buffer, 0, 0);
  wl_surface_damage_buffer(state->surface, 0, 0, state->width, state->height);
  wl_surface_commit(state->surface);
}
