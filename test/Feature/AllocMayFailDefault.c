// Without -alloc-may-fail (the default), malloc, calloc and realloc never
// return NULL: the branches on the failed allocations are dead and no
// .ktest has a `__bbk_alloc_fail` object.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck %s < %t.err
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck --check-prefix=KTEST %s
// RUN: not test -f %t.klee-out/test000002.ktest
// The same with the libc of KLEE (as bubaak runs KLEE)
// RUN: rm -rf %t.klee-out-libc
// RUN: %klee --output-dir=%t.klee-out-libc --exit-on-error -libc=klee %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck %s < %t.err
// RUN: not test -f %t.klee-out-libc/test000002.ktest

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  char *p = malloc(4);
  assert(p != NULL);
  char *q = calloc(2, 4);
  assert(q != NULL);
  q = realloc(q, 64);
  assert(q != NULL);
  free(q);
  free(p);
  printf("all allocations succeeded\n");
  return 0;
}

// OUT: all allocations succeeded
// CHECK-NOT: ASSERTION FAIL
// CHECK: KLEE: done: completed paths = 1
// KTEST: num objects: 0
// KTEST-NOT: __bbk_alloc_fail
