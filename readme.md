# bin2c

bin2c can be used to embed binary & text files inside C binaries.

It works by formatting the contents of the file in such a way that they
can be used inside a C89 compliant C string (albeit one that may be over the
C90 maximumg string length of 509 bytes).

This method of embedding binary data is highly portable and very fast (although
not as fast as using the linker to generate an object file directly). It is fast enough
to process even excessively large files (hundreds of MB) in two or three seconds.

## Setup

Compile and run:

```bash
$ make
$ ./build/bin2c
```

Compile and install:

```bash
$ make install prefix=/usr/local
```

Compile and run tests:

```bash
$ make test
```

Compile and run benchmarks:

```bash
$ make test
```

This will create a file with the benchmark results and print
averaged benchmark results on exit. If you want to regenerate
these avrages from the raw data manually at a later time, you
can use:

```bash
$ bash ./benchmark.sh evaluate < benchmark-2020-04-20-22:35:23.txt
```

## Examples

Compile `myfile.txt`:

```bash
$ bin2c myfile < myfile.txt > myfile.c
$ cc -c myfile.c -o myfile.o
```

Manually producing the C armor:

```bash
$ echo '#include <stdlib.h>'
$ echo -n 'const char myfile[] = "'
$ bin2c < myfile.txt
$ echo '";'
$ echo 'const size_t myfile_len = sizeof(myfile) - 1;';
```

Without using a temporary file:

```bash
$ bin2c myfile < myfile.txt | cc -x c -c - -o myfile.o
```

Printing the contents of the temporary file to stdout if the file
contains text:

```C
#include <stdio.h>

int main() {
  extern const char myfile[];
  printf("%s", myfile);
  return 0;
}
```

If the file contains binary data:

```C
#include <stdlib.h>
#include <stdio.h>

int main() {
  extern const char myfile[];
  extern const size_t myfile_len;
  fwrite(myfile, 1, myfile_len, stdout);
  return 0;
}
```

Usage in C++17 with string_view:

```C++
#include <cstdlib>
#include <string_view>
#include <iostream>

extern "C" const char myfile[];
extern "C" const size_t myfile_len;

int main() {
  std::string_view myfile_s{myfile, myfile_len};
  std::cout << myfile_s;
  return 0;
}
```

You could also convert the file to a different text encoding before embedding it:

```bash
$ iconv -f utf-8 -t utf-32 < myfile.txt \
  | bin2c myfile \
  | cc -x c -c - -o myfile.o
```

This is a more elaborate example; it is a makefile rule for embedding shaders.

```Makefile
build/shader_%.o: shaders/%.glsl
	$(CPP) -P -I "shaders/" $< \
	  | bin2c shader_$* \
		| $(CC) $(CPPFLAGS) $(CFLAGS) -x c -c - -o $@
```

The first line here defines the build pattern; generate an object file
called `shader_<name>` from a file called `<name>.glsl`.
Then we use the C preprocessor to add support for `#include` directives
inside our shader.

Then we perform the actual conversion to compilable C; the name of our
exported symbols is automatically calculated to be the same as our object
file.

Finally we compile the produced C code.

## Library usage

```C
// Header: bin2c.h, Lib: libbin2c.a

/// If this is defined, the bin2c function will included as an inline
/// function.
#define BIN2C_INLINE 1

/// If this is defined, bin2c can be used a header-only library.
/// Note that this enables an alternate, slower algorithm not based
/// on a lookup table.
#define BIN2C_HEADER_ONLY 1

/// Escape binary data
void bin2c(
  const uint8_t **in, const uint8_t *in_end,
  char **out, const char *out_end);
```

The bin2c function reads binary data from `[*in; in_end)`, escapes the
data and writes the resuling string to `[*out; out_end)`.
The function writes as many bytes as possible without overflowing either
in or out buffers. If necessary, the output will be truncated.

`in`/`out` are incremented to indicate the number of bytes red/written.

### Example

```C
// cc -c example.c -o example.o -I"${PWD}/src/"
// cc example.o build/libbin2c.a -o example
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <bin2c.h>
#include <stdio.h>

int main(int argc, const char **argv) {
  const char *in = argc > 1 ? argv[1] : "Hello World";
  char out[6];

  const char *ip = in;
  char *op = out;
  bin2c(
    (const uint8_t**)(&ip), (const uint8_t*)(in + strlen(in)),
    // Out buffer, leaving space for terminating zero byte
    &op, out+sizeof(out)-1);

  *op = '\0';

  printf("Converted `%s` to `%s`. Red %i bytes and wrote %i chars.%s\n",
    in, out, ip-in, op-out,
    (ip-in < strlen(in) ? " String was truncated." : ""));

  return 0;
}
```

## Comparison to other tools

The most common way of including a file in C is using xxd:

```
$ echo "const uint8_t myfile[] = {"
$ echo "foo" | xxd -i
$ echo "};"
```

This generates an array of hex characters:

```
const uint8_t myfile[] = {
  0x66, 0x6f, 0x6f, 0x0a
};
```

This method of embedding a file is both slower than using bin2c
and results in bigger source file sizes. xxd produces an overhead
of 6.2 bytes per byte while bin2c just needs 3.1 bytes for binary
and 1.2 bytes for text.

The format produced by bin2c is more readable for text, xxd's format
is more readable for binary. xxd also does not append a zero terminator.

The most efficient way of embedding binary data is just using ld to
directly produce an object file:

```bash
$ ld -r -b binary myfile -o myfile.o -m elf_x86_64
```

This method is not as portable as generating a source file, both because
ld needs to be available and you need to specify the architecture.
Using the produced symbols is also a bit more unwieldy:

```C
int main() {
  extern const char _binary_myfile_start, _binary_myfile_end;
  const char *myfile = &_binary_myfile_start;
  const size_t myfile_len = &_binary_myfile_end - &_binary_myfile_start;
  fwrite(myfile, 1, myfile_len, stdout);
  return 0;
}
```

Use ld, when portability is not an issue.

Use xxd, when

1. You really do not wish to include the zero terminator.
2. You are encoding a binary file and readability is important.

Use bin2c, when

1. You need a source file (just an object file will not do)
2. You are encoding text and readability is an issue
3. Encoding speed & efficiency are of importance to you.

## Encoding

Alphabetic letters, digits, SPACE, as well as the following symbols are
exported as is:

    !#%&'()*+,-./:;<=>?[]^_`{|}~

The following simple escape sequences are used:

    \a\b\t\v\f\r\"\\

Newline is encoded as a newline escape code, followed by an actual, escaped
newline in the source. This improves readability of text. E.g.

    This is the first line\n\
    This is the second.

All other characters are encoded as a three letter octal escape code:

    \000\001\002\003

This includes the zero terminator, `@`, `$` (which are not permitted
characters in C89) and `?` to avoid confusion with trigraphs.

### Text files

Text files stay mostly readable as long as only ascii characters are used.

### Unicode Support

No special steps are taken to encode unicode. The embedded file will use whatever
encoding the original file did use.

## Performance

The fastest way to embed a binary or text file is using the linker
directly (as shown above). Failing that, bin2c is quite quick.

bin2c is both quicker than xxd at actually generating the source file (by a
factor of around 30x to 40x) and both clang and gcc are faster at embedding
the source generated by bin2c (20x-30x faster than xxd generated source).

### Benchmarks

Benchmarks Recorded on a i7-9700K CPU @ 3.60GHz.
Bigger numbers are better.

Comparing raw xxd and bin2c encoding performance.

```
xxd                    13.9045  Mb/s
bin2c                  547.613  Mb/s
```

Comparing compilation speeds using xxd, bin2c and bypassing the compiler
althogether:

```
xxd_gcc                  0.918  Mb/s
xxd_clang                1.160  Mb/s
bin2c_gcc               18.879  Mb/s
bin2c_clang             32.761  Mb/s
compile_ld            1559.670  Mb/s
```

Comparing bin2c_gcc (generating the source file on the fly) and bin2c_gcc_baseline
compiling a cached bin2c generated source file. (Indicates how much impact improving
the performance of bin2c would have on the total compile time).

```
bin2c_gcc               18.879  Mb/s
bin2c_gcc_baseline      19.126  Mb/s
bin2c_clang             32.761  Mb/s
bin2c_clang_baseline    33.570  Mb/s
```

## LICENSE

Author: Karolin Varner <karo@cupdev.net>

Copyright 2019 Adobe. All rights reserved.
This file is licensed to you under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy
of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
OF ANY KIND, either express or implied. See the License for the specific language
governing permissions and limitations under the License.
