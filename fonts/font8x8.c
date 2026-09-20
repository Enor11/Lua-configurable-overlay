#include <stdint.h>
#include "font8x8.h"

#include "font8x8_basic.h"
#include "font8x8_control.h"
#include "font8x8_ext_latin.h"
#include "font8x8_greek.h"
#include "font8x8_misc.h"
#include "font8x8_box.h"
#include "font8x8_block.h"
#include "font8x8_hiragana.h"
#include "font8x8_sga.h"

uint32_t utf8_decode(const char *str) {
  if (!str || !*str) return '?';

  const unsigned char *s = (const unsigned char *)str;

  if (s[0] < 0x80) { // 1-byte ASCII
    return s[0];
  } 
    
  if ((s[0] & 0xE0) == 0xC0) { // 2-byte sequence
    if (!s[1]) return '?';
    return ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
  } 
    
  if ((s[0] & 0xF0) == 0xE0) { // 3-byte sequence (Box drawing, Hiragana, etc.)
    if (!s[1] || !s[2]) return '?';
    return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
  } 
    
  if ((s[0] & 0xF8) == 0xF0) { // 4-byte sequence
    if (!s[1] || !s[2] || !s[3]) return '?';
    return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
  }

  return '?';
}

const uint8_t *get_glyph(uint32_t c) {
    // Basic Latin (ASCII): U+0000 - U+007F
    if (c < 128)
        return (const uint8_t *)font8x8_basic[c];

    // Extended Latin: U+00A0 - U+00FF
    if (c >= 0x00A0 && c <= 0x00FF)
        return (const uint8_t *)font8x8_ext_latin[c - 0x00A0];

    // Greek Characters: U+0390 - U+03C9
    if (c >= 0x0390 && c <= 0x03C9)
        return (const uint8_t *)font8x8_greek[c - 0x0390];

    // Box Drawing Characters: U+2500 - U+257F
    if (c >= 0x2500 && c <= 0x257F)
        return (const uint8_t *)font8x8_box[c - 0x2500];

    // Block Elements: U+2580 - U+259F
    if (c >= 0x2580 && c <= 0x259F)
        return (const uint8_t *)font8x8_block[c - 0x2580];

    // Hiragana: U+3040 - U+309F
    if (c >= 0x3040 && c <= 0x309F)
        return (const uint8_t *)font8x8_hiragana[c - 0x3040];

    // Standard Galactic Alphabet (SGA): U+E541 - U+E55A
    if (c >= 0xE541 && c <= 0xE55A)
        return (const uint8_t *)font8x8_sga[c - 0xE541];

    // Fallback for missing/unsupported characters
    return (const uint8_t *)font8x8_basic['?'];
}
