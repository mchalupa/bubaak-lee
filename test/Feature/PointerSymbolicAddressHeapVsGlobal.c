// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// Symbolic-address behavior: a heap-allocated object must have an address
// distinct from any global variable (they are different live objects in
// different segments), so the equality of their addresses is infeasible and
// the asserts below never fork or fail. Their relative ORDER, however, remains
// symbolic, so an ordering comparison still forks.

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

int g1;
int g2;

int main() {
  char *h = malloc(1);

  // Distinct from every global.
  assert((uintptr_t)h != (uintptr_t)&g1);
  assert((uintptr_t)h != (uintptr_t)&g2);

  // Order relative to a global is still undetermined -> forks into two paths.
  int n = 0;
  if ((uintptr_t)h < (uintptr_t)&g1)
    ++n;

  // CHECK-NOT: ASSERTION FAIL
  // CHECK: KLEE: done: completed paths = 2
  // CHECK: KLEE: done: partially completed paths = 0
  return n;
}
