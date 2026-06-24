// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// Symbolic-address behavior: two distinct, simultaneously-live objects can
// never have the same address. In the segment-based memory model each object
// lives in its own segment with a symbolic address, but distinct live segments
// are constrained to be distinct, so the equality of their (integer) addresses
// is always false -- the asserts below hold on every path and never fork.

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

int main() {
  char *a = malloc(1);
  char *b = malloc(1);
  char *c = malloc(1);

  assert((uintptr_t)a != (uintptr_t)b);
  assert((uintptr_t)a != (uintptr_t)c);
  assert((uintptr_t)b != (uintptr_t)c);

  // None of the equalities is feasible, so nothing forks: a single path.
  // CHECK-NOT: ASSERTION FAIL
  // CHECK: KLEE: done: completed paths = 1
  // CHECK: KLEE: done: partially completed paths = 0
  return 0;
}
