// C standard library based IO. Private Header.
#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "bin2c_errors.h"

typedef FILE* b2c_file;

static inline b2c_file b2c_io_stdin() {
  return stdin;
}

static inline b2c_file b2c_io_stdout() {
  return stdout;
}

static inline void b2c_io_set_unbuffered(b2c_file fd) {
  setvbuf(fd, NULL, _IONBF, 0);
}

static inline ssize_t b2c_io_write(b2c_file fd, const char *buf, const char *end) {
  const char *ptr = buf;
  while (true) {
    size_t r = fwrite(ptr, 1, end-ptr, fd);
    ptr += r;

    if (ferror(fd) && errno == EINTR) continue;
    if (ferror(fd)) B2C_ABORT("Error writing data: %s\n", strerror(errno));
    break;
  }
  return ptr - buf;
}

static inline size_t b2c_io_read(b2c_file fd, char *buf, const char *end) {
  char *ptr = buf;
  while (true) {
    size_t r = fread(ptr, 1, end-ptr, fd);
    ptr += r;

    if (ferror(fd) && errno == EINTR) continue;
    if (ferror(fd)) B2C_ABORT("Error writing data: %s\n", strerror(errno));
    break;
  }
  return ptr - buf;
}
