// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// memcpy with a symbolic byte count must copy exactly that many bytes and leave
// the rest of the destination untouched.

#include "klee/klee.h"

#include <assert.h>
#include <string.h>

int main() {
  unsigned n;
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_assume(n <= 8);

  char src[8], dst[8];
  memset(src, 0xAB, sizeof(src));
  memset(dst, 0, sizeof(dst));

  memcpy(dst, src, n);

  if (n > 0)
    assert(dst[0] == (char)0xAB); // first byte copied when n > 0
  if (n < 8)
    assert(dst[7] == 0); // last byte untouched when fewer than 8 copied

  return 0;
}
