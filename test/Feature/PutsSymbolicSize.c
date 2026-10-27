// Printing from an object of a symbolic size cannot be executed natively, it
// is skipped if the result is not used (printing does not change the state)
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure %t.bc 2>&1 | FileCheck %s
// CHECK-NOT: symbolic-sized object
// CHECK: completed paths = 2
#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  unsigned n;
  klee_make_symbolic(&n, sizeof n, "n");
  klee_assume(n >= 2 & n < 16);
  char *p = malloc(n);
  p[0] = 'a';
  p[1] = '\0';
  puts(p);
  free(p);
  if (n > 8)
    return 1;
  return 0;
}
