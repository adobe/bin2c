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
  /// Lookup table char -> escaped for C.
  /// Each code is between one and 4 bytes
  extern const char bin2c_lookup_table_[];

  const uint32_t r = ((uint32_t*)bin2c_lookup_table_)[(uint8_t)chr];
  ((uint32_t*)out)[0] = r;

#if defined(__ORDER_LITTLE_ENDIAN__) && defined(__GNUC__) && defined(BIN2C_OPTIMIZE)
  // Branchless implementation of a size check here, based (usually)
  // BSR or LZCNT instruction. Depends on the endianess & on gcc
  return (32 - __builtin_clz(r) + 7) / 8;
#else
  return out[1] == 0 ? 1
       : out[2] == 0 ? 2
       : out[3] == 0 ? 3 : 4;
#endif
}
#endif

inline void bin2c(uint8_t **in, uint8_t *in_end, char **out, char *out_end) {
  assert(out_end-*out >= 4);
  // (hot loop) While data in inbuff & outbuf has 4 free slots
  // (bin2c needs four free slots)
  for (; *in < in_end && out_end-*out >= 4; (*in)++)
    *out += bin2c_single(**in, *out);
}
