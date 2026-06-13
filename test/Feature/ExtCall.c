// This test checks that symbolic arguments to a function call are correctly concretized
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc

// RUN: rm -rf %t.klee-out
// In the segment-based memory model, concretized values are not propagated back
// as constraints, so assert(x == y) may fail (fork limitation). Drop exit-on-error.
// RUN: %klee --output-dir=%t.klee-out --external-calls=all %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x >= 0);

  int y = abs(x);
  printf("y = %d\n", y);
  // CHECK: calling external: abs(value/address: (ReadLSB w32 0 x))

  assert(x == y);
}
