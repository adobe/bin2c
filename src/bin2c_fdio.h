// Direct file descriptor based IO. Private Header.
#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "bin2c_errors.h"

#ifdef __GNUC__
#define B2C_UNUSED __attribute__ ((unused))
#else
#define B2C_UNUSED
#endif

typedef int b2c_file;

static inline b2c_file b2c_io_stdin() {
  return STDIN_FILENO;
}

static inline b2c_file b2c_io_stdout() {
  return STDOUT_FILENO;
}

static inline void b2c_io_set_unbuffered(B2C_UNUSED b2c_file fd) {}

static inline ssize_t b2c_io_write(b2c_file fd, const char *buf, const char *end) {
  const char *ptr = buf;
  while (ptr != end) {
    ssize_t r = write(fd, ptr, end-ptr);
    if (r == 0) break; // EOF
    if (r == -1 && errno == EINTR) continue;
    if (r == -1) B2C_ABORT("Error writing data: %s\n", strerror(errno));
    ptr += r;
  }
  return ptr - buf;
}

static inline size_t b2c_io_read(b2c_file fd, char *buf, const char *end) {
  char *ptr = buf;
  while (ptr != end) {
    ssize_t r = read(fd, ptr, end-ptr);
    if (r == 0) break; // EOF
    if (r == -1 && errno == EINTR) continue;
    if (r == -1) B2C_ABORT("Error reading data: %s\n", strerror(errno));
    ptr += r;
  }
  return ptr - buf;
}
