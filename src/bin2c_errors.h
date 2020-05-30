// Error handling functionality. Private Header.
#pragma once
#include <stdio.h>

#define B2C_ABORT(...) { \
  fprintf(stderr, "Error: " __VA_ARGS__); \
  abort(); \
}


