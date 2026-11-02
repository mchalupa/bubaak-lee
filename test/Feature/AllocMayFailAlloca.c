// alloca, __builtin_alloca and stack variables never fail, also with
// -alloc-may-fail: no __bbk_alloc_fail objects, no forks.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail --exit-on-error %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck %s < %t.err
// RUN: not test -f %t.klee-out/test000002.ktest
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck --check-prefix=KTEST %s

#include <alloca.h>
#include <assert.h>
#include <stdio.h>

int main(void) {
  char buf[16];
  char *a = alloca(32);
  char *b = __builtin_alloca(64);
  assert(a != NULL);
  assert(b != NULL);
  a[31] = 1;
  b[63] = 2;
  buf[15] = 3;
  printf("stack allocations succeeded\n");
  return a[31] + b[63] + buf[15] - 6;
}

// OUT: stack allocations succeeded
// CHECK-NOT: ASSERTION FAIL
// CHECK: KLEE: done: completed paths = 1
// KTEST: num objects: 0
