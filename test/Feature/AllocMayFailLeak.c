// A failed allocation is not a leak (valid-memcleanup, valid-memtrack):
// with -alloc-may-fail, the program frees everything it allocated on every
// path. A leak on the successful path is still found.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail --check-memcleanup %t1.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ > %t.files
// RUN: not grep -q "leak.err" %t.files
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 -alloc-may-fail --check-leaks %t1.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out-2/ > %t.files2
// RUN: not grep -q "leak.err" %t.files2
// RUN: %clang %s -DLEAK -emit-llvm %O0opt -c -g -o %t2.bc
// RUN: rm -rf %t.klee-out-3
// RUN: %klee --output-dir=%t.klee-out-3 -alloc-may-fail --check-memcleanup %t2.bc 2>&1 | FileCheck --check-prefix=LEAK %s
// RUN: ls %t.klee-out-3/ > %t.files3
// RUN: grep -q "leak.err" %t.files3

#include <stdio.h>
#include <stdlib.h>

int *g;

int main(void) {
  int *p = malloc(sizeof(int));
  g = calloc(2, sizeof(int));
  if (p != NULL) {
    *p = 1;
#ifndef LEAK
    free(p);
#endif
  }
  char *r = malloc(8);
  if (r != NULL) {
    char *s = realloc(r, 16);
    if (s != NULL)
      r = s;
    free(r);
  }
  free(g);
  printf("done\n");
  return 0;
}

// CHECK-NOT: memory error
// CHECK-NOT: memory leak
// CHECK: KLEE: done: completed paths = {{[1-9][0-9]*}}
// LEAK: memory error: memory not cleaned up
