// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// Subtracting two pointers into the same object (same segment) with symbolic
// offsets must yield the difference of the offsets. This is the well-defined
// counterpart of the cross-object pointer subtraction that the segment model
// cannot represent (and which the optimizer must therefore not introduce).

#include "klee/klee.h"

#include <assert.h>
#include <stddef.h>

int arr[16];

int main() {
  unsigned i, j;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_make_symbolic(&j, sizeof(j), "j");
  klee_assume(i < 16);
  klee_assume(j < 16);

  int *pi = arr + i;
  int *pj = arr + j;

  ptrdiff_t d = pi - pj;
  assert(d == (ptrdiff_t)i - (ptrdiff_t)j);

  return 0;
}
