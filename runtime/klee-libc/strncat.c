/*===-- strncat.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <string.h>

char *strncat(char *dst, const char *src, size_t n) {
  char *d = dst;

  while (*d)
    ++d;
  while (n-- > 0 && *src)
    *d++ = *src++;
  *d = '\0';
  return dst;
}
