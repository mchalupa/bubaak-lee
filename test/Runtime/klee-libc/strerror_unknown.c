// strerror of a value out of the table: "Unknown error N" (an under-approximation)
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out
// RUN: not grep "ERROR" %t.out
// CHECK: KLEE: WARNING: under-approximation: strerror

#include "klee/klee.h"
#include <assert.h>
#include <string.h>

int main() {
  int e;
  klee_make_symbolic(&e, sizeof e, "e");
  klee_assume(e > 133 || e < 0);
  const char *m = strerror(e);
  if (strncmp(m, "Unknown error ", 14) != 0)
    klee_assert(0);
  if (strcmp(strerror(-5), "Unknown error -5") != 0)
    klee_assert(0);
  return 0;
}
