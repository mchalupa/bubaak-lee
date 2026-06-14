// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// Tests that KLEE correctly models aliasing between symbolic write and read
// indices: if the indices are equal the written value is visible; otherwise
// the original value is read.

#include "klee/klee.h"

#include <assert.h>

int arr[4];

int main() {
  unsigned i, j;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_make_symbolic(&j, sizeof(j), "j");
  klee_assume(i < 4);
  klee_assume(j < 4);

  arr[i] = 99;
  int val = arr[j];

  if (i == j)
    assert(val == 99);
  else
    assert(val == 0);

  return 0;
}
