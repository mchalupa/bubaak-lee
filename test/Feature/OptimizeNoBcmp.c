// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize %t.bc 2>&1 | FileCheck %s

// Under --optimize the pipeline would otherwise turn "memcmp(...) == 0" into a
// bcmp library call. The freestanding runtime is linked lazily before
// optimization, so a bcmp introduced afterwards would become an unresolved
// external call. optimizeModule registers a TargetLibraryAnalysis that marks
// bcmp unavailable to prevent this; this test guards that behavior.

#include "klee/klee.h"

#include <string.h>

int main() {
  char a[8], b[8];
  klee_make_symbolic(a, sizeof(a), "a");
  memset(b, 0, sizeof(b));

  int eq = (memcmp(a, b, sizeof(a)) == 0);

  // CHECK-NOT: calling external: bcmp
  return eq;
}
