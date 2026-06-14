// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// Tests that pointer arithmetic ordering within a single array is consistent
// with index ordering when both indices are symbolic.  All elements share the
// same segment so the comparison reduces to comparing byte offsets, which
// the solver can relate directly to i and j.

#include "klee/klee.h"

#include <assert.h>

int arr[8];

int main() {
  unsigned i, j;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_make_symbolic(&j, sizeof(j), "j");
  klee_assume(i < 8);
  klee_assume(j < 8);

  int *pi = arr + i;
  int *pj = arr + j;

  if (i < j)
    klee_assert(pi < pj);
  else if (i > j)
    klee_assert(pi > pj);
  else
    klee_assert(pi == pj);

  return 0;
}
