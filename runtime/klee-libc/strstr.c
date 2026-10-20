/*===-- strstr.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <string.h>

char *strstr(const char *haystack, const char *needle) {
  const char *h, *n;

  if (!*needle)
    return (char *)haystack;
  for (; *haystack; ++haystack) {
    for (h = haystack, n = needle; *h && *n && *h == *n; ++h, ++n)
      ;
    if (!*n)
      return (char *)haystack;
  }
  return (char *)0;
}
