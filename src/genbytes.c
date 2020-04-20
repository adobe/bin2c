#include <stdio.h>

/// Output all the bytes 0..255; used for testing bin2c
int main() {
  for (int i=0x00; i <= 0xff; i++) putchar(i);
  return 0;
}
