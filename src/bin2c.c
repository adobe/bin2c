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
    default: {} // pass
  }

  if (isprint(chr) && chr != '$' && chr != '@' && chr != '?') {
    out[0] = chr;
    return 1;
  } else {
    out[0] = '\\'; // octal encode
    out[1] = (chr >> 6 & 7) + '0';
    out[2] = (chr >> 3 & 7) + '0';
    out[3] = (chr >> 0 & 7) + '0';
    return 4;
  }
}

inline void bin2c(char **in, const char *in_end, char **out, char *out_end) {
  assert(out_end-*out >= 4);
  // (hot loop) While data in inbuff & outbuf has 4 free slots
  // (bin2c needs four free slots)
  for (; *in < in_end && out_end-*out > 4; (*in)++)
    *out += bin2c_single(**in, *out);
}

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
inline size_t b2c_fill(struct b2c_buf *buf) {
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
    bin2c(&ib.pt, ib.end, &ob.pt, ob.end); // hot loop
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
