// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// calloc must return zero-initialized memory. Read a symbolic in-bounds index
// and assert it is zero, so the check covers every byte of the allocation.

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>

int main() {
  unsigned i;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_assume(i < 8);

  char *p = calloc(8, 1);
  assert(p != 0);
  assert(p[i] == 0);

  free(p);
  return 0;
}
