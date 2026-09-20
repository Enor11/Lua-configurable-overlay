#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

uint32_t utf8_decode(const char *str);
const uint8_t *get_glyph(uint32_t c);

#endif
