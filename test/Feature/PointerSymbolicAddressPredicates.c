// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// Companion to PointerSymbolicAddressOrdering.c: checks that the remaining
// ordering predicates (<=, >, >=) on pointers into distinct objects (whose
// addresses are symbolic in the segment-based memory model) also fork the
// state. One anchor pointer is compared against three other, mutually
// unrelated pointers with a different predicate each, so all 2^3 = 8 outcome
// combinations are feasible.

#include "klee/klee.h"

#include <stdlib.h>

int main() {
  char *p = malloc(1);
  char *a = malloc(1);
  char *b = malloc(1);
  char *c = malloc(1);

  int n = 0;
  if (p <= a)
    ++n;
  if (p > b)
    ++n;
  if (p >= c)
    ++n;

  // CHECK: KLEE: done: completed paths = 8
  // CHECK: KLEE: done: partially completed paths = 0
  return n;
}
