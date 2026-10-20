/*===-- scanf.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* (stdio.h is not included: it redirects the names of the scanf functions to
 * __isoc99_*, both names are defined here) */

#include "nondet.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct _IO_FILE FILE;
extern FILE *stdin;
#define EOF (-1)

#define WEAK __attribute__((weak))

int __klee_libc_at_end(const void *stream);
void __klee_libc_set_end(const void *stream);
int __klee_libc_nondet_bool(const char *name);
#define at_end __klee_libc_at_end
#define set_end __klee_libc_set_end
#define nondet_bool __klee_libc_nondet_bool

/* --- formatted input ---------------------------------------------------- */

/* The conversions (in order) of the format that the model supports: the
 * integers (any value of the type, as from its decimal representation),
 * %c (any characters) and %s (a word of 1 to 64 characters, at most the
 * width). The model stops at the first unsupported conversion: the
 * conversions up to it may succeed, the next one fails (a matching
 * failure, a real behavior). Returns the number of assigned conversions or
 * EOF. */
#define MAX_WORD 64

static int scan(FILE *stream, const char *name, const char *fmt, va_list ap) {
  int assigned = 0, matched = 0, limit;
  const char *f = fmt;

  klee_report_under_approximation(name);
  if (at_end(stream) || nondet_bool(KLEE_MODEL("eof"))) {
    set_end(stream);
    return EOF;
  }
  /* the number of directives that succeed */
  limit = __klee_libc_nondet_int(KLEE_MODEL("scanf_ok"));
  klee_assume(limit >= 0);

  while (*f) {
    int suppress = 0, lng = 0;
    long width = -1;
    char conv;
    size_t size, i;

    if (*f != '%' || f[1] == '%') {
      f += (*f == '%') ? 2 : 1;
      continue; /* the input matches the literal characters */
    }
    ++f;
    if (*f == '*') {
      suppress = 1;
      ++f;
    }
    if (*f >= '0' && *f <= '9') {
      width = 0;
      while (*f >= '0' && *f <= '9')
        width = width * 10 + (*f++ - '0');
    }
    /* length: -2 hh, -1 h, 1 l, 2 ll, 3 j, 4 z, 5 t */
    if (*f == 'h') {
      lng = -1;
      if (*++f == 'h') {
        lng = -2;
        ++f;
      }
    } else if (*f == 'l') {
      lng = 1;
      if (*++f == 'l') {
        lng = 2;
        ++f;
      }
    } else if (*f == 'q' || *f == 'L') {
      lng = 2;
      ++f;
    } else if (*f == 'j') {
      lng = 3;
      ++f;
    } else if (*f == 'z') {
      lng = 4;
      ++f;
    } else if (*f == 't') {
      lng = 5;
      ++f;
    }
    conv = *f;
    if (!conv)
      break;
    ++f;

    if (conv == 'd' || conv == 'i' || conv == 'u' || conv == 'o' ||
        conv == 'x' || conv == 'X') {
      size = lng == -2   ? sizeof(char)
             : lng == -1 ? sizeof(short)
             : lng == 1  ? sizeof(long)
             : lng == 2  ? sizeof(long long)
             : lng == 3  ? sizeof(intmax_t)
             : lng >= 4  ? sizeof(size_t)
                         : sizeof(int);
      if (matched >= limit)
        break;
      ++matched;
      if (!suppress) {
        __klee_libc_write_symbolic(va_arg(ap, void *), size,
                                   KLEE_MODEL("scanf"));
        ++assigned;
      }
    } else if (conv == 'c' && lng == 0) {
      if (matched >= limit)
        break;
      ++matched;
      if (!suppress) {
        __klee_libc_write_symbolic(va_arg(ap, char *),
                                   width < 0 ? 1 : (size_t)width,
                                   KLEE_MODEL("scanf"));
        ++assigned;
      }
    } else if (conv == 's' && lng == 0) {
      size_t max = width < 0 || width > MAX_WORD ? MAX_WORD : (size_t)width;
      if (matched >= limit)
        break;
      ++matched;
      if (!suppress) {
        char *s = va_arg(ap, char *);
        size_t len = __klee_libc_nondet_size(KLEE_MODEL("scanf_len"));
        klee_assume(len >= 1 & len <= max);
        __klee_libc_write_symbolic(s, len, KLEE_MODEL("scanf"));
        for (i = 0; i < len; ++i)
          klee_assume(s[i] != ' ' & s[i] != '\t' & s[i] != '\n' &
                      s[i] != '\v' & s[i] != '\f' & s[i] != '\r' &
                      s[i] != '\0');
        s[len] = '\0';
        ++assigned;
      }
    } else {
      /* not supported: this directive fails */
      break;
    }
  }
  return assigned;
}

WEAK int vfscanf(FILE *stream, const char *fmt, va_list ap) {
  return scan(stream, "fscanf", fmt, ap);
}

WEAK int fscanf(FILE *stream, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = scan(stream, "fscanf", fmt, ap);
  va_end(ap);
  return ret;
}

WEAK int __isoc99_fscanf(FILE *stream, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = scan(stream, "fscanf", fmt, ap);
  va_end(ap);
  return ret;
}

WEAK int scanf(const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = scan(stdin, "scanf", fmt, ap);
  va_end(ap);
  return ret;
}

WEAK int __isoc99_scanf(const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = scan(stdin, "scanf", fmt, ap);
  va_end(ap);
  return ret;
}

/* The result of sscanf is determined by the string, which is not modelled:
 * the path is not explored further. */
WEAK int sscanf(const char *str, const char *fmt, ...) {
  (void)str;
  (void)fmt;
  __klee_libc_unsupported("sscanf");
  return EOF;
}

WEAK int __isoc99_sscanf(const char *str, const char *fmt, ...) {
  (void)str;
  (void)fmt;
  __klee_libc_unsupported("sscanf");
  return EOF;
}

WEAK int swscanf(const wchar_t *str, const wchar_t *fmt, ...) {
  (void)str;
  (void)fmt;
  __klee_libc_unsupported("swscanf");
  return EOF;
}

WEAK int __isoc99_swscanf(const wchar_t *str, const wchar_t *fmt, ...) {
  (void)str;
  (void)fmt;
  __klee_libc_unsupported("swscanf");
  return EOF;
}
