// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// Symbolic-address behavior: a heap object allocated after another has been
// freed CAN occupy the same address as the freed one (the address is free to
// be reused). The distinctness constraint only applies to objects that are
// live at the same time; once an object is freed its segment is gone and a
// later allocation's symbolic address is unconstrained with respect to it.
//
// We capture the freed object's address as an integer before freeing it (no
// use-after-free of the pointer), then compare it against a later allocation's
// address. Both "same address" and "different address" are feasible, so the
// comparison forks into two completed paths.

#include "klee/klee.h"

#include <stdint.h>
#include <stdlib.h>

int main() {
  char *a = malloc(1);
  uintptr_t ia = (uintptr_t)a;
  free(a);

  char *b = malloc(1);
  uintptr_t ib = (uintptr_t)b;

  int n = 0;
  if (ia == ib) // reuse of the freed address is feasible -> forks
    ++n;

  // CHECK: KLEE: done: completed paths = 2
  // CHECK: KLEE: done: partially completed paths = 0
  return n;
}
