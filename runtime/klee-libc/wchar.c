/*===-- wchar.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* The functions on wide strings. They are weak, so that the definitions in
 * the program take precedence. */

#include <stddef.h>
#include <wchar.h>

#define WEAK __attribute__((weak))

WEAK size_t wcslen(const wchar_t *s) {
  size_t len = 0;
  while (s[len])
    ++len;
  return len;
}

WEAK size_t wcsnlen(const wchar_t *s, size_t maxlen) {
  size_t len = 0;
  while (len < maxlen && s[len])
    ++len;
  return len;
}

WEAK wchar_t *wcscpy(wchar_t *dst, const wchar_t *src) {
  wchar_t *d = dst;
  while ((*d++ = *src++))
    ;
  return dst;
}

WEAK wchar_t *wcsncpy(wchar_t *dst, const wchar_t *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; ++i)
    dst[i] = src[i];
  for (; i < n; ++i)
    dst[i] = L'\0';
  return dst;
}

WEAK wchar_t *wcscat(wchar_t *dst, const wchar_t *src) {
  wchar_t *d = dst;
  while (*d)
    ++d;
  while ((*d++ = *src++))
    ;
  return dst;
}

WEAK wchar_t *wcsncat(wchar_t *dst, const wchar_t *src, size_t n) {
  wchar_t *d = dst;
  while (*d)
    ++d;
  while (n-- > 0 && *src)
    *d++ = *src++;
  *d = L'\0';
  return dst;
}

WEAK int wcscmp(const wchar_t *s1, const wchar_t *s2) {
  for (; *s1 == *s2; ++s1, ++s2)
    if (*s1 == L'\0')
      return 0;
  return *s1 < *s2 ? -1 : 1;
}

WEAK int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
  for (; n > 0; ++s1, ++s2, --n) {
    if (*s1 != *s2)
      return *s1 < *s2 ? -1 : 1;
    if (*s1 == L'\0')
      return 0;
  }
  return 0;
}

WEAK wchar_t *wcschr(const wchar_t *s, wchar_t c) {
  for (;; ++s) {
    if (*s == c)
      return (wchar_t *)s;
    if (*s == L'\0')
      return (wchar_t *)0;
  }
}

WEAK wchar_t *wcsrchr(const wchar_t *s, wchar_t c) {
  const wchar_t *last = (wchar_t *)0;
  for (;; ++s) {
    if (*s == c)
      last = s;
    if (*s == L'\0')
      return (wchar_t *)last;
  }
}

WEAK wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i)
    s[i] = c;
  return s;
}

WEAK wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i)
    dst[i] = src[i];
  return dst;
}

WEAK wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, size_t n) {
  size_t i;
  if (dst < src) {
    for (i = 0; i < n; ++i)
      dst[i] = src[i];
  } else {
    for (i = n; i > 0; --i)
      dst[i - 1] = src[i - 1];
  }
  return dst;
}

WEAK int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i)
    if (s1[i] != s2[i])
      return s1[i] < s2[i] ? -1 : 1;
  return 0;
}

WEAK wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i)
    if (s[i] == c)
      return (wchar_t *)(s + i);
  return (wchar_t *)0;
}
