#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <assert.h>
#include <unistd.h>

#include "bin2c.h"

// Freebsd apparently behaves weirdly with unbuffered stdio…
// https://www.reddit.com/r/C_Programming/comments/gsxvh3/how_to_analyze_assembly_code_to_guide/fs8yyst/
#if (defined(__unix__) && !defined(BIN2C_FORCE_STDIO)) || defined(BIN2C_FORCE_FD_IO)
#include "bin2c_fdio.h"
#else
#include "bin2c_stdio.h"
#endif

struct b2c_buf {
  b2c_file fd;
  // Start pointer, end pointer, fill level
  char *start, *end, *pt;
};

struct b2c_buf b2c_buf_create(b2c_file fd, size_t len) {
  b2c_io_set_unbuffered(fd);
  struct b2c_buf r;
  r.fd = fd;
  r.start = r.pt = (char*)malloc(len);
  r.end = r.start + len;
  return r;
}

// fread but directly aborts execution on error when nothing was red.
// Automatically resets the buffer pointer.
// Behaviour is undefined if buffer is not entirely empty
size_t b2c_fill(struct b2c_buf *buf) {
  // PT is the read pointer for the input buffer, so we cannot use
  // it to indicate data size (end indicates allocation size).
  // We could however move any remaining data to the start of the buffer,
  // but it's not really worth it……
  assert(buf->pt == buf->start || buf->pt == buf->end);

  size_t red = b2c_io_read(buf->fd, buf->start, buf->end);

  buf->pt = buf->end - red;
  if (buf->pt != buf->start) // read was shorter than buffer, move data to end
    memmove(buf->pt, buf->start, red);
  return red;
}

// fwrite but directly aborts execution on error or EOF.
// Automatically resets the buffer pointer.
void b2c_flush(struct b2c_buf *buf) {
  b2c_io_write(buf->fd, buf->start, buf->pt);
  buf->pt = buf->start;
}

void b2c_puts(struct b2c_buf *buf, const char *str) {
  while (true) {
    for (; buf->pt < buf->end; buf->pt++, str++) {
      if (*str == '\0')
        return;
      *buf->pt = *str;
    }
    b2c_flush(buf);
  }
}

int help() {
  extern const char blob_help[];
  extern const size_t blob_help_len;
  fwrite(blob_help, 1, blob_help_len, stdout);
  return 0;
}

int main(int argc, const char **argv) {
  const char *var_name = NULL;
  if (argc == 1) {
    /* pass */
  } else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    return help();
  } else if (argc == 2 && strcmp(argv[1], "-h") == 0) {
    return help();
  } else if (argc == 2) {
    var_name = argv[1];
  } else {
    fprintf(stderr, "error: Invalid number of arguments\n\n");
    help();
    return 4;
  }

  setlocale(LC_ALL, "C");
  struct b2c_buf ib = b2c_buf_create(b2c_io_stdin(), 8129);
  struct b2c_buf ob = b2c_buf_create(b2c_io_stdout(), 8129);

  if (var_name != NULL) {
    b2c_puts(&ob, "#include <stdlib.h>\n");
    b2c_puts(&ob, "const char ");
    b2c_puts(&ob, var_name);
    b2c_puts(&ob, "[] = \"\\\n");
  }

  while (true) {
    b2c_flush(&ob);
    if (ib.pt == ib.start || ib.pt == ib.end) { // read buf must be empty
      if (b2c_fill(&ib) == 0) {
        break; // EOF
      }
    }
    bin2c((const uint8_t**)&ib.pt, (const uint8_t*)ib.end, &ob.pt, ob.end); // hot loop
  }

  if (var_name != NULL) {
    b2c_puts(&ob, "\";\n");
    b2c_puts(&ob, "const size_t ");
    b2c_puts(&ob, var_name);
    b2c_puts(&ob, "_len = sizeof(");
    b2c_puts(&ob, var_name);
    b2c_puts(&ob, ") - 1;\n");
  }
  
  b2c_flush(&ob);

  return 0;
}
