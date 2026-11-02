// A failed realloc returns NULL and keeps the old object; realloc(NULL, n)
// may fail as malloc; a failed allocation is not a leak.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail --error-fn=reach_error --check-leaks --check-memcleanup %t.bc > %t.log 2> %t.err
// RUN: not grep -q "KLEE: ERROR" %t.err
// RUN: grep -q "first failed" %t.log
// RUN: grep -q "realloc failed, kept 42 7" %t.log
// RUN: grep -q "realloc succeeded, 42 7" %t.log
// RUN: grep -q "completed paths = 3" %t.err
// RUN: %ktest-tool %t.klee-out/test000003.ktest | grep -c "name: '__bbk_alloc_fail'$" | grep -qx 2
#include <stdio.h>
#include <stdlib.h>

void reach_error(void);

int main(void) {
  int *p = realloc(NULL, 2 * sizeof(int));
  if (!p) {
    printf("first failed\n");
    return 0;
  }
  p[0] = 42;
  p[1] = 7;
  int *q = realloc(p, 100 * sizeof(int));
  if (!q) {
    if (p[0] != 42 || p[1] != 7)
      reach_error();
    printf("realloc failed, kept %d %d\n", p[0], p[1]);
    free(p);
    return 0;
  }
  printf("realloc succeeded, %d %d\n", q[0], q[1]);
  free(q);
  return 0;
}
