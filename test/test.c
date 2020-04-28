#include <assert.h>
#include <string.h>
#include "bin2c.h"

#define TEST(in, outlen, ref, il, ol) { \
  char out[512]; \
  memset(out, 0, sizeof(out)); \
  const char *ip = in; \
  char *op = out; \
  bin2c( \
    (const uint8_t**)&ip, (const uint8_t*)(in+strlen(in)), \
    &op, out+outlen); \
  assert(strcmp(out, ref) == 0); \
  assert(ip-in == il); \
  assert(op-out == ol); \
}
  

int main() {
  TEST("", 0,    "", 0, 0);
  TEST("", 1,    "", 0, 0);
  TEST("", 2,    "", 0, 0);

  TEST("Hello", 0,    "", 0, 0);
  TEST("Hello", 1,    "H", 1, 1);
  TEST("Hello", 2,    "He", 2, 2);
  TEST("Hello", 3,    "Hel", 3, 3);
  TEST("Hello", 4,    "Hell", 4, 4);
  TEST("Hello", 5,    "Hello", 5, 5);
  TEST("Hello", 6,    "Hello", 5, 5);
  TEST("Hello", 7,    "Hello", 5, 5);

  TEST("\"\n$a", 0,    "", 0, 0);
  TEST("\"\n$a", 1,    "", 0, 0);
  TEST("\"\n$a", 2,    "\\\"", 1, 2);
  TEST("\"\n$a", 3,    "\\\"", 1, 2);
  TEST("\"\n$a", 4,    "\\\"", 1, 2);
  TEST("\"\n$a", 5,    "\\\"", 1, 2);
  TEST("\"\n$a", 6,    "\\\"\\n\\\n", 2, 6);
  TEST("\"\n$a", 7,    "\\\"\\n\\\n", 2, 6);
  TEST("\"\n$a", 8,    "\\\"\\n\\\n", 2, 6);
  TEST("\"\n$a", 9,    "\\\"\\n\\\n", 2, 6);
  TEST("\"\n$a", 10,   "\\\"\\n\\\n\\044", 3, 10);
  TEST("\"\n$a", 11,   "\\\"\\n\\\n\\044a", 4, 11);
  TEST("\"\n$a", 12,   "\\\"\\n\\\n\\044a", 4, 11);
  TEST("\"\n$a", 13,   "\\\"\\n\\\n\\044a", 4, 11);
  TEST("\"\n$a", 14,   "\\\"\\n\\\n\\044a", 4, 11);
}
