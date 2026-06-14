// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// memmove must handle overlapping source and destination correctly (as if via
// a temporary buffer), for both forward and backward overlap.

#include "klee/klee.h"

#include <assert.h>
#include <string.h>

int main() {
  // Forward overlap: dst starts after src.
  char f[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  memmove(f + 2, f, 6);
  // f[2..7] become the old f[0..5].
  assert(f[2] == 0 && f[3] == 1 && f[7] == 5);
  // The bytes before the destination are untouched.
  assert(f[0] == 0 && f[1] == 1);

  // Backward overlap: dst starts before src.
  char b[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  memmove(b, b + 2, 6);
  // b[0..5] become the old b[2..7].
  assert(b[0] == 2 && b[5] == 7);
  // The bytes after the destination are untouched.
  assert(b[6] == 6 && b[7] == 7);

  return 0;
}
