/*===-- snprintf.c --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* A model of the printing to strings (sprintf, snprintf, ...). The writes to
 * the buffer go through KLEE, so that they are checked (calling the function
 * of the host would not check them and would not see the symbolic data).
 *
 * The output is exact for the integers (also symbolic: the digits are
 * expressions over the value, no forking), the characters and the strings.
 * The conversions that are not modelled (floating-point numbers, %p, wide
 * characters, the grouping flag) stop the path (an under-approximation,
 * reported). The functions are weak, so that the definitions in the program
 * take precedence. */

#include "nondet.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define WEAK __attribute__((weak))

struct output {
  char *buf;
  size_t cap; /* the size of buf (the output is truncated to cap - 1) */
  size_t pos; /* the number of characters printed so far */
};

static void put(struct output *o, char c) {
  if (o->pos + 1 < o->cap)
    o->buf[o->pos] = c;
  ++o->pos;
}

static void put_n(struct output *o, char c, long n) {
  for (; n > 0; --n)
    put(o, c);
}

struct spec {
  int left, plus, space, alt, zero;
  long width;
  long prec; /* -1 if not given */
};

static void pad_before(struct output *o, const struct spec *s, long len) {
  if (!s->left && s->width > len)
    put_n(o, ' ', s->width - len);
}

static void pad_after(struct output *o, const struct spec *s, long len) {
  if (s->left && s->width > len)
    put_n(o, ' ', s->width - len);
}

static void print_integer(struct output *o, const struct spec *s,
                          unsigned long long v, int negative, unsigned base,
                          int upper, int is_signed) {
  unsigned long long p;
  long ndigits = 1, zeros = 0, len, i, k;
  const char *prefix = "";
  long prefixlen = 0;

  /* the number of digits (forks on the magnitude of a symbolic value) */
  for (p = base; v >= p; p *= base) {
    ++ndigits;
    if (p > ULLONG_MAX / base)
      break;
  }
  if (s->prec == 0 && v == 0)
    ndigits = 0;

  if (is_signed) {
    if (negative)
      prefix = "-";
    else if (s->plus)
      prefix = "+";
    else if (s->space)
      prefix = " ";
  } else if (s->alt && base == 16 && v != 0) {
    prefix = upper ? "0X" : "0x";
  } else if (s->alt && base == 8) {
    /* the first digit is zero */
    if ((v != 0 || ndigits == 0) && s->prec <= ndigits)
      zeros = 1;
  }
  while (prefix[prefixlen])
    ++prefixlen;

  if (s->prec > ndigits)
    zeros = s->prec - ndigits;
  len = prefixlen + zeros + ndigits;
  if (s->zero && !s->left && s->prec < 0 && s->width > len) {
    zeros += s->width - len;
    len = s->width;
  }

  pad_before(o, s, len);
  for (i = 0; i < prefixlen; ++i)
    put(o, prefix[i]);
  put_n(o, '0', zeros);
  for (i = ndigits - 1; i >= 0; --i) {
    unsigned long long d = v;
    for (k = 0; k < i; ++k)
      d /= base;
    d %= base;
    /* no branching on the digit */
    put(o, (char)('0' + d + (d > 9) * ((upper ? 'A' : 'a') - '0' - 10)));
  }
  pad_after(o, s, len);
}

static void print_string(struct output *o, const struct spec *s,
                         const char *str) {
  long len = 0, i;

  if (!str) /* as glibc */
    str = s->prec >= 0 && s->prec < 6 ? "" : "(null)";
  while ((s->prec < 0 || len < s->prec) && str[len])
    ++len;
  pad_before(o, s, len);
  for (i = 0; i < len; ++i)
    put(o, str[i]);
  pad_after(o, s, len);
}

WEAK int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
  struct output o = {buf, size, 0};
  const char *f = fmt;

  while (*f) {
    struct spec s = {0, 0, 0, 0, 0, 0, -1};
    int lng = 0; /* -2 hh, -1 h, 1 l, 2 ll, 3 j, 4 z, 5 t, 6 L */
    char conv;

    if (*f != '%') {
      put(&o, *f++);
      continue;
    }
    ++f;

    /* flags */
    for (;; ++f) {
      if (*f == '-')
        s.left = 1;
      else if (*f == '+')
        s.plus = 1;
      else if (*f == ' ')
        s.space = 1;
      else if (*f == '#')
        s.alt = 1;
      else if (*f == '0')
        s.zero = 1;
      else if (*f == '\'')
        __klee_libc_unsupported("sprintf"); /* grouping (the locale) */
      else
        break;
    }
    /* width */
    if (*f == '*') {
      int w = va_arg(ap, int);
      if (w < 0) {
        s.left = 1;
        w = -w;
      }
      s.width = w;
      ++f;
    } else {
      while (*f >= '0' && *f <= '9')
        s.width = s.width * 10 + (*f++ - '0');
    }
    /* precision */
    if (*f == '.') {
      ++f;
      s.prec = 0;
      if (*f == '*') {
        int p = va_arg(ap, int);
        s.prec = p < 0 ? -1 : p;
        ++f;
      } else {
        while (*f >= '0' && *f <= '9')
          s.prec = s.prec * 10 + (*f++ - '0');
      }
    }
    /* length */
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
    } else if (*f == 'q') {
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
    } else if (*f == 'L') {
      lng = 6;
      ++f;
    }

    conv = *f;
    if (conv == '\0')
      break;
    ++f;

    switch (conv) {
    case 'd':
    case 'i': {
      long long v;
      if (lng == 2 || lng == 6)
        v = va_arg(ap, long long);
      else if (lng == 1)
        v = va_arg(ap, long);
      else if (lng == 3)
        v = va_arg(ap, intmax_t);
      else if (lng == 4 || lng == 5)
        v = va_arg(ap, ptrdiff_t);
      else
        v = va_arg(ap, int);
      if (lng == -1)
        v = (short)v;
      else if (lng == -2)
        v = (signed char)v;
      if (v < 0)
        print_integer(&o, &s, 0ULL - (unsigned long long)v, 1, 10, 0, 1);
      else
        print_integer(&o, &s, (unsigned long long)v, 0, 10, 0, 1);
      break;
    }
    case 'u':
    case 'o':
    case 'x':
    case 'X': {
      unsigned long long v;
      if (lng == 2 || lng == 6)
        v = va_arg(ap, unsigned long long);
      else if (lng == 1)
        v = va_arg(ap, unsigned long);
      else if (lng == 3)
        v = va_arg(ap, uintmax_t);
      else if (lng == 4 || lng == 5)
        v = va_arg(ap, size_t);
      else
        v = va_arg(ap, unsigned int);
      if (lng == -1)
        v = (unsigned short)v;
      else if (lng == -2)
        v = (unsigned char)v;
      print_integer(&o, &s, v, 0, conv == 'u' ? 10 : (conv == 'o' ? 8 : 16),
                    conv == 'X', 0);
      break;
    }
    case 'c': {
      char c;
      if (lng == 1)
        __klee_libc_unsupported("sprintf"); /* a wide character */
      c = (char)va_arg(ap, int);
      pad_before(&o, &s, 1);
      put(&o, c);
      pad_after(&o, &s, 1);
      break;
    }
    case 's':
      if (lng == 1) {
        /* a wide string: the multibyte conversion is not modelled */
        __klee_libc_unsupported("sprintf");
        const wchar_t *ws = va_arg(ap, const wchar_t *);
        long len = 0, i;
        if (!ws) {
          print_string(&o, &s, (const char *)0);
          break;
        }
        while ((s.prec < 0 || len < s.prec) && ws[len])
          ++len;
        pad_before(&o, &s, len);
        for (i = 0; i < len; ++i)
          put(&o, (char)ws[i]);
        pad_after(&o, &s, len);
      } else {
        print_string(&o, &s, va_arg(ap, const char *));
      }
      break;
    case 'p':
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
      /* the address or the digits are not known */
      __klee_libc_unsupported("sprintf");
      break;
    case 'n': {
      void *p = va_arg(ap, void *);
      if (lng == -2)
        *(signed char *)p = (signed char)o.pos;
      else if (lng == -1)
        *(short *)p = (short)o.pos;
      else if (lng == 1)
        *(long *)p = (long)o.pos;
      else if (lng == 2)
        *(long long *)p = (long long)o.pos;
      else if (lng == 3)
        *(intmax_t *)p = (intmax_t)o.pos;
      else if (lng == 4 || lng == 5)
        *(ptrdiff_t *)p = (ptrdiff_t)o.pos;
      else
        *(int *)p = (int)o.pos;
      break;
    }
    case '%':
      put(&o, '%');
      break;
    default:
      /* an unknown conversion, print it as it is */
      __klee_libc_unsupported("sprintf");
      put(&o, '%');
      put(&o, conv);
    }
  }

  if (o.cap > 0)
    o.buf[o.pos < o.cap ? o.pos : o.cap - 1] = '\0';
  return o.pos > INT_MAX ? -1 : (int)o.pos;
}

WEAK int snprintf(char *buf, size_t size, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return ret;
}

WEAK int vsprintf(char *buf, const char *fmt, va_list ap) {
  return vsnprintf(buf, SIZE_MAX, fmt, ap);
}

WEAK int sprintf(char *buf, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = vsnprintf(buf, SIZE_MAX, fmt, ap);
  va_end(ap);
  return ret;
}

/* The allocation succeeds (as malloc in KLEE) */
WEAK int vasprintf(char **strp, const char *fmt, va_list ap) {
  va_list ap2;
  int n;
  va_copy(ap2, ap);
  n = vsnprintf((char *)0, 0, fmt, ap2);
  va_end(ap2);
  if (n < 0)
    return -1;
  *strp = (char *)malloc((size_t)n + 1);
  if (!*strp)
    return -1;
  return vsnprintf(*strp, (size_t)n + 1, fmt, ap);
}

WEAK int asprintf(char **strp, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = vasprintf(strp, fmt, ap);
  va_end(ap);
  return ret;
}
