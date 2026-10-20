/*===-- socket.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* Sockets: creating a socket or a connection fails (a real behavior, e.g.,
 * when no more file descriptors are available, but only one of the
 * behaviors: an under-approximation). inet_addr is exact. The functions are
 * weak, so that the definitions in the program take precedence. */

#include "nondet.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define WEAK __attribute__((weak))

static int fail(const char *name, int err) {
  klee_report_under_approximation(name);
  errno = err;
  return -1;
}

WEAK int socket(int domain, int type, int protocol) {
  (void)domain;
  (void)type;
  (void)protocol;
  return fail("socket", EMFILE);
}

WEAK int connect(int fd, const struct sockaddr *addr, socklen_t len) {
  (void)fd;
  (void)addr;
  (void)len;
  return fail("connect", ECONNREFUSED);
}

WEAK int bind(int fd, const struct sockaddr *addr, socklen_t len) {
  (void)fd;
  (void)addr;
  (void)len;
  return fail("bind", EADDRINUSE);
}

WEAK int listen(int fd, int n) {
  (void)fd;
  (void)n;
  return fail("listen", EADDRINUSE);
}

WEAK int accept(int fd, struct sockaddr *addr, socklen_t *len) {
  (void)fd;
  (void)addr;
  (void)len;
  return fail("accept", EMFILE);
}

/* the dotted-decimal notation a.b.c.d (each part at most 255, decimal) */
WEAK in_addr_t inet_addr(const char *cp) {
  unsigned parts[4];
  int i;
  for (i = 0; i < 4; ++i) {
    unsigned v = 0, digits = 0;
    if (cp[0] == '0' && cp[1] >= '0' && cp[1] <= '9')
      goto general; /* octal */
    while (*cp >= '0' && *cp <= '9' && digits < 4) {
      v = v * 10 + (unsigned)(*cp++ - '0');
      ++digits;
    }
    if (digits == 0 || digits > 3 || v > 255)
      goto general;
    parts[i] = v;
    if (i < 3) {
      if (*cp != '.')
        goto general;
      ++cp;
    }
  }
  if (*cp != '\0')
    goto general;
  return htonl((parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) |
               parts[3]);
general:
  /* other forms (octal, hexadecimal, fewer parts, ...) are not modelled */
  __klee_libc_unsupported("inet_addr");
  return (in_addr_t)-1;
}
