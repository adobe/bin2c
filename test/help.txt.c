#include <stdlib.h>
const char myfile[] = "\
NAME\n\
\n\
  bin2c - Embed text files in C code.\n\
\n\
SYNOPSIS\n\
\n\
  (1) bin2c < RAW_FILE > ENCODED\n\
  (2) bin2c VAR_NAME < RAW_FILE > C_FILE\n\
  (3) bin2c [-h|--help]\n\
\n\
DESCRIPTION\n\
\n\
  (1) The first form reads data from stdin and output a string suitable for\n\
      pasting into a C string.\n\
  (2) Generates a C file that exports two symbols when compiled:\n\
      `const char <VAR_NAME>[]` and `const size_t <VAR_NAME>_len`.\n\
      Len is the length of the original file, excluding the terminating\n\
      zero character.\n\
  (3) Display this help\n\
\n\
  The format produced by bin2c is suitable for embedding into a C89 source file\n\
  (if the compiler supports overlong strings).  numbers, alphabetic letters,\n\
  and some symbols are embedded as is. Double quotes are escaped. Newline is\n\
  encoded as \\n followed by an escaped new line to improve readability.  Single\n\
  character escape codes (e.g. \\t) are encoded as such and all other characters\n\
  are printed in octal.\n\
\n\
  A terminating zero character is added to the file.\n\
\n\
  bin2c itself is not unicode-aware (multibyte unicode characters are encoded as\n\
  multiple, separate symbols). This is because C89 does not actually permit any\n\
  non ascii characters. This should not deter you from embedding unicode however;\n\
  the binary data in your executable will contain exactly the same data as the original\n\
  file, so it will be whatever encoding the original file was.\n\
";
const size_t myfile_len = sizeof(myfile) - 1;
