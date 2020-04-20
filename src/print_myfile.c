#include <stdlib.h>
#include <stdio.h>

/// Main function used during tests to check if the file produced is
/// the same as the input
int main() {
  extern const char myfile[];
  extern const size_t myfile_len;
  fwrite(myfile, 1, myfile_len, stdout);
  return 0;
}

