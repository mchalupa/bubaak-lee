// A dereference of the NULL of a failed allocation is a memory error (the
// pointer is null), the failed allocation is not a leak; alloca never fails.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail --check-leaks %t.bc > %t.log 2> %t.err
// RUN: grep -q "KLEE: ERROR: .*AllocFailureNull.c:21: memory error: null page access" %t.err
// RUN: grep -c "KLEE: ERROR" %t.err | grep -qx 1
// RUN: grep -q "completed paths = 1" %t.err
// RUN: ls %t.klee-out | not grep -q leak
// RUN: %ktest-tool %t.klee-out/test000001.ktest > %t.ktest
// RUN: %ktest-tool %t.klee-out/test000002.ktest >> %t.ktest
// RUN: grep -c "name: '__bbk_alloc_fail'$" %t.ktest | grep -qx 2
#include <alloca.h>
#include <stdlib.h>

int main(void) {
  int *s = alloca(sizeof(int));
  *s = 1;
  int *b = __builtin_alloca(sizeof(int));
  *b = 2;
  int *p = malloc(sizeof(int));
  *p = *s + *b;
  free(p);
  return 0;
}
