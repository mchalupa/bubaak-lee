/*===-- strdup.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stdlib.h>
#include <string.h>

__attribute__((weak)) char *strdup(const char *s) {
  size_t len = 0, i;
  char *res;

  while (s[len])
    ++len;
  res = (char *)malloc(len + 1);
  if (!res)
    return (char *)0;
  for (i = 0; i <= len; ++i)
    res[i] = s[i];
  return res;
}

__attribute__((weak)) char *strndup(const char *s, size_t n) {
  size_t len = 0, i;
  char *res;

  while (len < n && s[len])
    ++len;
  res = (char *)malloc(len + 1);
  if (!res)
    return (char *)0;
  for (i = 0; i < len; ++i)
    res[i] = s[i];
  res[len] = '\0';
  return res;
}
