// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// Tests '<' (ordering) comparison of pointers into distinct objects. In the
// segment-based memory model such pointers live in different segments whose
// addresses are symbolic, so an ordering comparison cannot be decided
// statically and must fork the state.
//
// Here one anchor pointer is compared against three other, mutually unrelated
// pointers. Because the three comparisons are independent (the three targets
// are never compared against each other, so transitivity does not constrain
// them), all 2^3 = 8 combinations of outcomes are feasible.

#include "klee/klee.h"

#include <stdlib.h>

int main() {
  char *p = malloc(1);
  char *a = malloc(1);
  char *b = malloc(1);
  char *c = malloc(1);

  int n = 0;
  if (p < a)
    ++n;
  if (p < b)
    ++n;
  if (p < c)
    ++n;

  // CHECK: KLEE: done: completed paths = 8
  // CHECK: KLEE: done: partially completed paths = 0
  return n;
}
