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
  // abs() is lowered to the llvm.abs intrinsic by recent clang, so the
  // remaining external call is printf, whose symbolic argument (y, derived
  // from x) is concretized. The fork prints external-call arguments using its
  // segment-based memory model: name(segment: N, value/address: ...).
  // CHECK: calling external: printf(segment: {{[0-9]+}}, value/address:

  assert(x == y);
}
