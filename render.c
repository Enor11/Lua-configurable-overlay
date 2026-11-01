#include <string.h>
#include <stdlib.h>
#include "render.h"
#include "lua_helper.h"
#include "fonts/font8x8.h"

void draw_filled_rect(uint32_t *buffer, int buf_width, int buf_height, int x, int y, int w, int h, uint32_t color) {
	for (int dy = 0; dy < h; dy++) {
		int py = y + dy;
		if (py < 0 || py >= buf_height) continue;
		for (int dx = 0; dx < w; dx++) {
			int px = x + dx;
			if (px < 0 || px >= buf_width) continue;
			buffer[py * buf_width + px] = color;
		}
	}
}

void draw_line(uint32_t *buffer, int buf_width, int buf_height, int x0, int y0, int x1, int y1, int thickness, uint32_t color) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	int half = thickness / 2;

	while (1) {
		for (int ty = -half; ty < thickness - half; ty++) {
			for (int tx = -half; tx < thickness - half; tx++) {
				int px = x0 + tx;
				int py = y0 + ty;
				if (px >= 0 && px < buf_width && py >= 0 && py < buf_height) {
					buffer[py * buf_width + px] = color;
				}
			}
		}

		if (x0 == x1 && y0 == y1) break;

		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void draw_char_glyph(uint32_t *buffer, int buf_width, int buf_height, const char *str, int start_x, int y, int scale, uint32_t color) {
	if (!str) return;
	int cur_x = start_x;
	const char *p = str;

	while (*p) {
		uint32_t codepoint = utf8_decode(p);
		const uint8_t *glyph = get_glyph(codepoint);

		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				if (glyph[row] & (1 << col)) {
					for (int sy = 0; sy < scale; sy++) {
						for (int sx = 0; sx < scale; sx++) {
							int px = cur_x + (col * scale) + sx;
							int py = y + (row * scale) + sy;
							if (px >= 0 && px < buf_width && py >= 0 && py < buf_height) {
								buffer[py * buf_width + px] = color;
							}
						}
					}
				}
			}
		}

		unsigned char c = *p;
		if (c < 0x80) p += 1;
		else if ((c & 0xE0) == 0xC0) p += 2;
		else if ((c & 0xF0) == 0xE0) p += 3;
		else if ((c & 0xF8) == 0xF0) p += 4;
		else p += 1;

		cur_x += 8 * scale;
	}
}

void draw_frame(struct wayland_state *state) {
	int pixel_count = state->width * state->height;
	trigger_update_event(state->L);
	memcpy(state->shm_data, state->offscreen_buffer, pixel_count * sizeof(uint32_t));

	wl_surface_attach(state->surface, state->buffer, 0, 0);
	wl_surface_damage_buffer(state->surface, 0, 0, state->width, state->height);
	wl_surface_commit(state->surface);
}
