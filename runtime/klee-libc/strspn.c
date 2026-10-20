/*===-- strspn.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* strspn, strcspn, strpbrk and strtok (exact). The functions are weak, so
 * that the definitions in the program take precedence. */

#include <string.h>

#define WEAK __attribute__((weak))

static int in_set(char c, const char *set) {
  for (; *set; ++set)
    if (*set == c)
      return 1;
  return 0;
}

WEAK size_t strspn(const char *s, const char *accept) {
  size_t n = 0;
  while (s[n] && in_set(s[n], accept))
    ++n;
  return n;
}

WEAK size_t strcspn(const char *s, const char *reject) {
  size_t n = 0;
  while (s[n] && !in_set(s[n], reject))
    ++n;
  return n;
}

WEAK char *strpbrk(const char *s, const char *accept) {
  for (; *s; ++s)
    if (in_set(*s, accept))
      return (char *)s;
  return (char *)0;
}

WEAK char *strtok_r(char *str, const char *delim, char **saveptr) {
  char *end;
  if (!str)
    str = *saveptr;
  str += strspn(str, delim);
  if (*str == '\0') {
    *saveptr = str;
    return (char *)0;
  }
  end = str + strcspn(str, delim);
  if (*end == '\0') {
    *saveptr = end;
    return str;
  }
  *end = '\0';
  *saveptr = end + 1;
  return str;
}

WEAK char *strtok(char *str, const char *delim) {
  static char *saved;
  return strtok_r(str, delim, &saved);
}
