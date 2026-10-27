// The branch of the conditional operator is folded into a select (no fork),
// with --cover-selects its sides get separate paths while a side is not
// covered
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-FOLD
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --cover-selects %t.bc 2>&1 | FileCheck %s
// RUN: %klee-stats --print-columns 'BCov(%)' --table-format=csv %t.klee-out | FileCheck %s -check-prefix=CHECK-STATS
// CHECK-FOLD: completed paths = 1
// CHECK: completed paths = 2
// CHECK-STATS: BCov(%)
// CHECK-STATS-NEXT: 100.00
#include "klee/klee.h"

int main(void) {
  int y = 0;
  // the select forks only in the first iteration, later both of its sides
  // are covered (2 paths, not 8)
  for (int i = 0; i < 3; ++i) {
    int x = klee_int("x");
    y += x > 5 ? 3 : 7;
  }
  return y;
}
