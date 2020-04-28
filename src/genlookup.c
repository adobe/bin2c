#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "bin2c.h"

int main() {
  uint8_t table[1024];
  memset(table, 0, sizeof(table));

  for (int i=0x00; i <= 0xff; i++) {
    uint8_t in = i;
    const uint8_t *inpt = &in;
    uint8_t *outp = &table[i*4];
    bin2c(&inpt, inpt+1, (char**)&outp, (char*)outp+4);

    // Store the length of the table in the two most
    // significant bits of the last char
    uint8_t len = outp - &table[i*4] - 1;
    table[i*4 + 3] |= len << 6;
  }

  if (fwrite(table, 1, sizeof(table), stdout) != sizeof(table)) {
    fprintf(stderr, "Error writing table: %s\n", strerror(errno));
    abort();
  }

  return 0;
}
