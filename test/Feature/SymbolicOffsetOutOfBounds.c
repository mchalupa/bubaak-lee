// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// A store through a pointer with a fully symbolic offset must fork: the
// in-bounds offsets succeed while the out-of-bounds offsets are reported as a
// memory error. Exercises the segment bounds check on a symbolic offset.

#include "klee/klee.h"

int a[4];

int main() {
  unsigned i;
  klee_make_symbolic(&i, sizeof(i), "i");

  // i is unconstrained: a[i] is valid for i in 0..3 and out of bounds
  // otherwise.
  a[i] = 1;

  // CHECK-DAG: memory error: out of bound pointer
  // CHECK-DAG: KLEE: done: completed paths = 1
  return 0;
}
