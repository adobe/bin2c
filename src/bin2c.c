#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>

#define BIN2C_IO_ERRCK(v) \
  if ((v) == EOF) { \
    fprintf(stderr, "error: %s", strerror(errno)); \
    abort(); \
  }

void bin2c() {
  for (int inc = getchar(); inc != EOF; inc = getchar()) {
    switch ((char)inc) {
      case '\a': BIN2C_IO_ERRCK(fputs("\\a", stdout));     continue;
      case '\b': BIN2C_IO_ERRCK(fputs("\\b", stdout));     continue;
      case '\t': BIN2C_IO_ERRCK(fputs("\\t", stdout));     continue;
      case '\n': BIN2C_IO_ERRCK(fputs("\\n\\\n", stdout)); continue;
      case '\v': BIN2C_IO_ERRCK(fputs("\\v", stdout));     continue;
      case '\f': BIN2C_IO_ERRCK(fputs("\\f", stdout));     continue;
      case '\r': BIN2C_IO_ERRCK(fputs("\\r", stdout));     continue;
      case '\\': BIN2C_IO_ERRCK(fputs("\\\\", stdout));    continue;
      case '"':  BIN2C_IO_ERRCK(fputs("\\\"", stdout));    continue;
      default: {} // pass
    }

    if (isprint(inc) && inc != '$' && inc != '@' && inc != '?') {
      BIN2C_IO_ERRCK(putchar(inc));
    } else {
      BIN2C_IO_ERRCK(printf("\\%.3o", inc))
    }
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

  if (var_name != NULL)
    printf(
      "#include <stdlib.h>\n"
      "const char %s[] = \"\\\n", var_name);

  bin2c();

  if (var_name != NULL)
    printf("\";\nconst size_t %s_len = sizeof(%s) - 1;\n", var_name, var_name);

  fflush(stdout);

  return 0;
}
