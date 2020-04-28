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

#include "bin2c.h"

struct b2c_buf {
  FILE *stream;
  // Start pointer, end pointer, fill level
  char *start, *end, *pt;
};

inline struct b2c_buf b2c_buf_create(FILE *stream, size_t len) {
  struct b2c_buf r;
  r.stream = stream;
  r.start = r.pt = (char*)malloc(len);
  r.end = r.start + len;
  return r;
}

// fread but directly aborts execution on error when nothing was red.
// Automatically resets the buffer pointer.
// Behaviour is undefined if buffer is not entirely empty
size_t b2c_fill(struct b2c_buf *buf) {
  assert(buf->pt == buf->start || buf->pt == buf->end);
  size_t red = fread(buf->start, 1, buf->end - buf->start, buf->stream);
  if (red == 0 && ferror(buf->stream)) {
    fprintf(stderr, "Error reading data: %s\n", strerror(errno));
    abort();
  }
  buf->pt = buf->end - red;
  if (buf->pt != buf->start) // read was shorter than buffer
    memmove(buf->pt, buf->start, red);
  return red;
}

// fwrite but directly aborts execution on error or EOF.
// Automatically resets the buffer pointer.
void b2c_flush(struct b2c_buf *buf) {
  size_t cnt = buf->pt - buf->start;
  if (fwrite(buf->start, 1, cnt, buf->stream) != cnt) {
    fprintf(stderr, "Error writing data: %s\n", strerror(errno));
    abort();
  }
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

inline int help() {
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
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  struct b2c_buf ib = b2c_buf_create(stdin, 8129);
  struct b2c_buf ob = b2c_buf_create(stdout, 8129);

  if (var_name != NULL) {
    b2c_puts(&ob, "#include <stdlib.h>\n");
    b2c_puts(&ob, "const char ");
    b2c_puts(&ob, var_name);
    b2c_puts(&ob, "[] = \"\\\n");
  }

  while (!feof(stdin) || (ib.pt != ib.start && ib.pt != ib.end)) {
    if (ib.pt == ib.start || ib.pt == ib.end) b2c_fill(&ib);
    b2c_flush(&ob);
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
