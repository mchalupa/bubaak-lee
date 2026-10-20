/*===-- input.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* Models of the functions that read the input of the program (files, the
 * standard input, sockets). The input is unknown, so the data are symbolic,
 * but only the results that the real function can return for some input
 * are explored: the reads succeed (no I/O errors) and the end of a stream
 * (or a closed connection) is sticky. The models are under-approximations
 * (they are reported as such). The functions are weak, so that the
 * definitions in the program take precedence. */

#include "nondet.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <wchar.h>

#define WEAK __attribute__((weak))

/* the streams (or sockets) that reached the end */
#define MAX_ENDED 32
static const void *ended[MAX_ENDED];
static unsigned num_ended;

int __klee_libc_at_end(const void *stream) {
  unsigned i;
  for (i = 0; i < num_ended; ++i)
    if (ended[i] == stream)
      return 1;
  return 0;
}

void __klee_libc_set_end(const void *stream) {
  if (__klee_libc_at_end(stream))
    return;
  if (num_ended == MAX_ENDED)
    __klee_libc_unsupported("too many streams");
  ended[num_ended++] = stream;
}

int __klee_libc_nondet_bool(const char *name) {
  return __klee_libc_nondet_int(name) != 0;
}

/* --- characters --------------------------------------------------------- */

static int read_char(FILE *stream, const char *name) {
  int c;
  klee_report_under_approximation(name);
  if (__klee_libc_at_end(stream) || __klee_libc_nondet_bool(KLEE_MODEL("eof"))) {
    __klee_libc_set_end(stream);
    return EOF;
  }
  c = __klee_libc_nondet_int(KLEE_MODEL("char"));
  klee_assume(c >= 0 & c <= 255);
  return c;
}

WEAK int fgetc(FILE *stream) { return read_char(stream, "fgetc"); }
WEAK int getc(FILE *stream) { return read_char(stream, "getc"); }
WEAK int _IO_getc(FILE *stream) { return read_char(stream, "getc"); }
WEAK int getc_unlocked(FILE *stream) { return read_char(stream, "getc"); }
WEAK int fgetc_unlocked(FILE *stream) { return read_char(stream, "fgetc"); }
WEAK int getchar(void) { return read_char(stdin, "getchar"); }
WEAK int getchar_unlocked(void) { return read_char(stdin, "getchar"); }

/* --- lines -------------------------------------------------------------- */

/* Read a line: len characters (1 <= len <= n - 1) followed by '\0'. Only
 * the last character can be '\n'; if the line is shorter than n - 1 and
 * does not end with '\n', the stream ended. The characters can be '\0'
 * (binary input). Only s[0..len] is written (as by the real function). */
static char *read_line(char *s, int n, FILE *stream, const char *name) {
  size_t len, i;
  char last;

  klee_report_under_approximation(name);
  if (n <= 0)
    return (char *)0;
  if (n == 1) {
    s[0] = '\0';
    return s;
  }
  if (__klee_libc_at_end(stream) || __klee_libc_nondet_bool(KLEE_MODEL("eof"))) {
    /* end of the stream before reading anything: s is not changed */
    __klee_libc_set_end(stream);
    return (char *)0;
  }

  len = __klee_libc_nondet_size(KLEE_MODEL("line_len"));
  klee_assume(len >= 1 & len <= (size_t)n - 1);
  __klee_libc_write_symbolic(s, len, KLEE_MODEL("line"));
  for (i = 0; i + 1 < len; ++i)
    klee_assume(s[i] != '\n');
  last = s[len - 1];
  if (len < (size_t)n - 1 && last != '\n')
    __klee_libc_set_end(stream);
  s[len] = '\0';
  return s;
}

WEAK char *fgets(char *s, int n, FILE *stream) {
  return read_line(s, n, stream, "fgets");
}

WEAK char *fgets_unlocked(char *s, int n, FILE *stream) {
  return read_line(s, n, stream, "fgets");
}

/* The wide characters of the input in the "C" locale (ASCII) */
WEAK wchar_t *fgetws(wchar_t *s, int n, FILE *stream) {
  size_t len, i;

  klee_report_under_approximation("fgetws");
  if (n <= 0)
    return (wchar_t *)0;
  if (n == 1) {
    s[0] = L'\0';
    return s;
  }
  if (__klee_libc_at_end(stream) || __klee_libc_nondet_bool(KLEE_MODEL("eof"))) {
    __klee_libc_set_end(stream);
    return (wchar_t *)0;
  }

  len = __klee_libc_nondet_size(KLEE_MODEL("line_len"));
  klee_assume(len >= 1 & len <= (size_t)n - 1);
  for (i = 0; i < len; ++i) {
    wchar_t c;
    klee_make_symbolic(&c, sizeof(c), KLEE_MODEL("wchar"));
    klee_assume(c >= 0 & c <= 127);
    if (i + 1 < len)
      klee_assume(c != L'\n');
    s[i] = c;
  }
  if (len < (size_t)n - 1 && s[len - 1] != L'\n')
    __klee_libc_set_end(stream);
  s[len] = L'\0';
  return s;
}

/* --- blocks ------------------------------------------------------------- */

/* Either all items are read, or none (the end of the stream) */
WEAK size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  klee_report_under_approximation("fread");
  if (size == 0 || nmemb == 0)
    return 0;
  if (__klee_libc_at_end(stream) || __klee_libc_nondet_bool(KLEE_MODEL("eof"))) {
    __klee_libc_set_end(stream);
    return 0;
  }
  __klee_libc_write_symbolic(ptr, size * nmemb, KLEE_MODEL("fread"));
  return nmemb;
}

WEAK size_t fread_unlocked(void *ptr, size_t size, size_t nmemb,
                           FILE *stream) {
  return fread(ptr, size, nmemb, stream);
}

/* --- sockets ------------------------------------------------------------ */

/* The connection was closed (0) or 1..len bytes were received (at most
 * MAX_RECV bytes, or exactly len bytes, to limit the forking). */
#define MAX_RECV 256

static ssize_t receive(int sockfd, void *buf, size_t len, const char *name) {
  size_t ret;
  klee_report_under_approximation(name);
  if (len == 0)
    return 0;
  if (__klee_libc_at_end((const void *)(intptr_t)(sockfd + 1)) ||
      __klee_libc_nondet_bool(KLEE_MODEL("closed"))) {
    __klee_libc_set_end((const void *)(intptr_t)(sockfd + 1));
    return 0;
  }
  ret = __klee_libc_nondet_size(KLEE_MODEL("recv_len"));
  klee_assume((ret >= 1) & ((ret <= MAX_RECV) | (ret == len)) & (ret <= len));
  __klee_libc_write_symbolic(buf, ret, KLEE_MODEL("recv"));
  return (ssize_t)ret;
}

WEAK ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  (void)flags;
  return receive(sockfd, buf, len, "recv");
}

WEAK ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                      struct sockaddr *src_addr, socklen_t *addrlen) {
  (void)flags;
  (void)addrlen;
  if (src_addr)
    __klee_libc_unsupported("recvfrom"); /* the address is not modelled */
  return receive(sockfd, buf, len, "recvfrom");
}


/* --- streams ------------------------------------------------------------ */

/* Opening a file fails (a real behavior, e.g., the file does not exist,
 * but only one of them) */
WEAK FILE *fopen(const char *path, const char *mode) {
  (void)path;
  (void)mode;
  klee_report_under_approximation("fopen");
  errno = ENOENT;
  return (FILE *)0;
}

WEAK FILE *fopen64(const char *path, const char *mode) {
  return fopen(path, mode);
}

WEAK FILE *fdopen(int fd, const char *mode) {
  (void)fd;
  (void)mode;
  klee_report_under_approximation("fdopen");
  errno = EBADF;
  return (FILE *)0;
}

/* The models of reading never fail (and the writing is not modelled) */
WEAK int ferror(FILE *stream) {
  (void)stream;
  return 0;
}

WEAK int feof(FILE *stream) { return __klee_libc_at_end(stream); }

WEAK void clearerr(FILE *stream) {
  unsigned i;
  for (i = 0; i < num_ended; ++i) {
    if (ended[i] == stream) {
      ended[i] = ended[--num_ended];
      return;
    }
  }
}

WEAK int fflush(FILE *stream) {
  (void)stream;
  return 0;
}
