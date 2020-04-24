#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef BIN2C_BOOTSTRAP
inline size_t b2c_strcpy(char *from, char *to) {
    char *pt = from;
    for (; *pt != '\0'; pt++, to++) *to = *pt;
    return pt - from;
}

inline size_t bin2c_single(uint8_t chr, char *out) {
  switch ((char)chr) {
    case '\a': return b2c_strcpy("\\a", out);
    case '\b': return b2c_strcpy("\\b", out);
    case '\t': return b2c_strcpy("\\t", out);
    case '\n': return b2c_strcpy("\\n\\\n", out);
    case '\v': return b2c_strcpy("\\v", out);
    case '\f': return b2c_strcpy("\\f", out);
    case '\r': return b2c_strcpy("\\r", out);
    case '\\': return b2c_strcpy("\\\\", out);
    case '"':  return b2c_strcpy("\\\"", out);
    case '$':
    case '@':
    case '?':
      goto octal;
  }

  // isprint, but inline
  if (chr >= ' ' && chr <= '~') {
    out[0] = chr;
    return 1;
  }

octal:
  out[0] = '\\'; // octal encode
  out[1] = (chr >> 6 & 7) + '0';
  out[2] = (chr >> 3 & 7) + '0';
  out[3] = (chr >> 0 & 7) + '0';
  return 4;
}

#else
inline size_t bin2c_single(uint8_t chr, char *out) {
  extern const char bin2c_lookup_table_[];

  // NOTE: The length is in the two most significant bits of
  // the last character of the output. The length in there is
  // 0..3, our actual value is 1..4 so we need to add 1.
  // In order to store this mask in an endianess independent way
  // we store it a byte array; in order to apply it efficiently
  // we cast to an uint32_t so we can perform a one 4 byte instruction
  // instead of multiple single byte instructions when copying into
  // the output buffer.

  /// Lookup table char -> escaped for C.
  /// Each code is between one and 4 bytes and padded by zeroes.
  const uint8_t *slot8 = ((uint8_t*)bin2c_lookup_table_) + chr*4;

  // The bit mask for erasing the length information
  uint8_t mask8[] = { 0xff, 0xff, 0xff, 0x3f };

  // Copy the data from the lookup table and erase length info
  *((uint32_t*)out) = *((uint32_t*)slot8) & *((uint32_t*)mask8);

  // Extract length info from lookup
  return ((slot8[3] & 0xc0) >> 6) + 1;
}
#endif

inline void bin2c(uint8_t **in, uint8_t *in_end, char **out, char *out_end) {
  assert(out_end-*out >= 4);
  // (hot loop) While data in inbuff & outbuf has 4 free slots
  // (bin2c needs four free slots)
  for (; *in < in_end && out_end-*out >= 4; (*in)++)
    *out += bin2c_single(**in, *out);
}
