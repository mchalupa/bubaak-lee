// The allocations may fail with -alloc-may-fail (the input __bbk_alloc_fail
// per call), the tests follow the failures when seeded or replayed.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc

// Without -alloc-may-fail, nothing changes
// RUN: rm -rf %t.klee-plain && %klee --output-dir=%t.klee-plain --error-fn=reach_error %t.bc > %t.log 2> %t.err
// RUN: not grep -q "reach_error called" %t.err
// RUN: grep -q "completed paths = 1" %t.err
// RUN: not grep -q "__bbk_alloc_fail" %t.klee-plain/test000001.ktest

// With it, the guard is reachable (the second allocation fails)
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail --error-fn=reach_error %t.bc > %t.log 2> %t.err
// RUN: grep -q "reach_error called" %t.err
// RUN: grep -q "malloc failed" %t.log
// RUN: grep -q "both allocated" %t.log
// RUN: test -f %t.klee-out/test000003.assert.err
// RUN: %ktest-tool %t.klee-out/test000003.ktest > %t.ktest
// RUN: grep -c "name: '__bbk_alloc_fail'$" %t.ktest | grep -qx 2
// RUN: grep -A1 "object 1: name: '__bbk_alloc_fail'" %t.ktest | grep -q "size: 1"

// A seed with the failure follows it
// RUN: rm -rf %t.klee-seed && %klee --output-dir=%t.klee-seed --alloc-may-fail --error-fn=reach_error --seed-file=%t.klee-out/test000003.ktest --named-seed-matching --named-seed-matching-strict --drop-exhausted-seeds --only-replay-seeds --only-seed %t.bc > %t.log 2> %t.err
// RUN: grep -q "reach_error called" %t.err
// RUN: not grep -q "both allocated" %t.log

// A seed without the decisions takes the allocations as succeeded
// RUN: rm -rf %t.klee-seed && %klee --output-dir=%t.klee-seed --alloc-may-fail --error-fn=reach_error --seed-file=%t.klee-plain/test000001.ktest --named-seed-matching --named-seed-matching-strict --drop-exhausted-seeds --only-replay-seeds --only-seed %t.bc > %t.log 2> %t.err
// RUN: not grep -q "reach_error called" %t.err
// RUN: grep -q "both allocated" %t.log

// -replay-nondets follows it too
// RUN: rm -rf %t.klee-replay && %klee --output-dir=%t.klee-replay --alloc-may-fail --error-fn=reach_error --replay-nondets=%t.klee-out/test000003.ktest %t.bc > %t.log 2> %t.err
// RUN: grep -q "reach_error called" %t.err
// RUN: not grep -q "both allocated" %t.log
// RUN: grep -q "completed paths = 0" %t.err
#include <stdio.h>
#include <stdlib.h>

void reach_error(void);

int main(void) {
  // (a failure of the first allocation ends the program)
  int *p = malloc(sizeof(int));
  if (!p) {
    printf("malloc failed\n");
    return 0;
  }
  *p = 1;
  int *q = calloc(2, sizeof(int));
  if (!q) {
    free(p);
    reach_error();
    return 1;
  }
  printf("both allocated\n");
  free(q);
  free(p);
  return 0;
}
