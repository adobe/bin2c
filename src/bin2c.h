#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// This header comes in the following modes:
// Normal header: Just declares bin2c.
//  BIN2C_HEADER_ONLY=undefined BIN2C_INLINE=undefined BIN2C_OBJECT_FILE=undefined
// Object file: This is basically as the implementation of libbin2c.c.
//  This exports the bin2c symbol…
//  BIN2C_OBJECT_FILE=1 BIN2C_INLINE=any BIN2C_HEADER_ONLY=any
// Header Only
//   This pulls in the slower header only/bootstrapping implementation as
//   an inline function.
//   BIN2C_HEADER_ONLY=1 BIN2C_OBJECT_FILE=undefined BIN2C_INLINE=any
// Inline
//   BIN2C_INLINE=any BIN2C_OBJECT_FILE=undefined BIN2C_HEADER_ONLY=undefined

#if !defined(BIN2C_INLINE) && !defined(BIN2C_HEADER_ONLY) && !defined(BIN2C_OBJECT_FILE)
extern void bin2c(const uint8_t **in, const uint8_t *in_end, char **out, const char *out_end);
#endif

// bin2c_single, header only and bootstrapping implementation.
// This does not depend on a lookup table and so is used to generate the lookup table.
// It is also used in the header only variant.
#if defined(BIN2C_HEADER_ONLY) && !defined(BIN2C_OBJECT_FILE)

static inline size_t bin2c_single(uint8_t chr, char *out) {
  switch ((char)chr) {
#define B2C_ESCAPE_MAP(chr, escape) \
    case chr : memcpy(out, escape, strlen(escape)); return strlen(escape);
    B2C_ESCAPE_MAP('\a', "\\a");
    B2C_ESCAPE_MAP('\b', "\\b");
    B2C_ESCAPE_MAP('\t', "\\t");
    B2C_ESCAPE_MAP('\v', "\\v");
    B2C_ESCAPE_MAP('\f', "\\f");
    B2C_ESCAPE_MAP('\r', "\\r");
    B2C_ESCAPE_MAP('\\', "\\\\");
    B2C_ESCAPE_MAP('\n', "\\n\\\n");
    B2C_ESCAPE_MAP('"',  "\\\"");
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
#endif

// bin2c_single inline/object file variant requiring a lookup table
#if defined(BIN2C_OBJECT_FILE) || (defined(BIN2C_INLINE) && !defined(BIN2C_HEADER_ONLY))
static inline size_t bin2c_single(uint8_t chr, char *out) {
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

  // The bit mask for erasing the length information; stored as a byte
  // array instead of a uint32_t literal to avoid endianess issues
  uint8_t mask8[] = { 0xff, 0xff, 0xff, 0x3f };

  // Copy the data from the lookup table and erase length info
  // This really is just `out = slot & mask;`.
  // Both casting to a packed struct and using memcpy are ways to express
  // unaligned 32 bit pointer access; this is faster than doing the copying
  // byte by byte on x86_64
#ifdef __GNUC__
  struct __attribute__((__packed__)) uint32_noalign {
    uint32_t v;
  };
  ((struct uint32_noalign*)out)->v = ((struct uint32_noalign*)slot8)->v & ((struct uint32_noalign*)mask8)->v;
#else
  uint32_t mask32, slot32;
  memcpy(&mask32, mask8, 4);
  memcpy(&slot32, slot8, 4);
  slot32 &= mask32;
  memcpy(out, &slot32, 4);
#endif

  // Extract length info from lookup
  return ((slot8[3] & 0xc0) >> 6) + 1;
}
#endif

#if defined(BIN2C_OBJECT_FILE) || defined(BIN2C_INLINE) || defined(BIN2C_HEADER_ONLY)
#if defined(BIN2C_INLINE) || defined(BIN2C_HEADER_ONLY)
static inline
#endif
void bin2c(const uint8_t **in, const uint8_t *in_end, char **out, const char *out_end) {
  // (hot loop) While data in inbuff & outbuf has 4 free slots
  // (bin2c needs four free slots)
  for (; *in < in_end && out_end-*out >= 4; (*in)++)
    *out += bin2c_single(**in, *out);

  // Edge case handling (processing the last three bytes)
  for (; *in < in_end; (*in)++) {
    char buf[4];
    int len = bin2c_single(**in, (char*)&buf);
    int avail = out_end-*out;
    if (len > avail) return;
    memcpy(*out, buf, len);
    *out += len;
  }
}
#endif
