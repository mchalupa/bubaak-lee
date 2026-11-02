// With -alloc-may-fail, calloc fails if the size overflows.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail %t.bc > %t.log 2> %t.err
// RUN: not grep -q "KLEE: ERROR" %t.err
// RUN: grep -q "overflow: null" %t.log
// RUN: not grep -q "overflow: allocated" %t.log
// RUN: grep -q "small: allocated" %t.log
// RUN: grep -q "small: null" %t.log
#include "klee/klee.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  size_t n;
  klee_make_symbolic(&n, sizeof n, "n");
  klee_assume(n > 1);
  klee_assume(n < 8);
  char *p = calloc(n, SIZE_MAX / 2 + 1);
  printf("overflow: %s\n", p ? "allocated" : "null");
  char *q = calloc(n, 4);
  printf("small: %s\n", q ? "allocated" : "null");
  if (q && q[n * 4 - 1] != 0)
    return 1;
  free(q);
  return 0;
}
