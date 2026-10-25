// A state killed because a query timed out gets a test, also if the test
// is written only as an incomplete or kill test, and the exploration is
// reported as incomplete (the path is partially completed)
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --max-solver-time=1s --write-incomplete-tests %t.bc 2> %t1.err
// RUN: FileCheck --check-prefix=CHECK-ERR --input-file=%t1.err %s
// RUN: FileCheck --check-prefix=CHECK-SUMMARY --input-file=%t1.err %s
// RUN: ls %t.klee-out-1 | grep ".solver.err"
// RUN: %ktest-tool %t.klee-out-1/test000002.ktest | FileCheck --check-prefix=CHECK-TEST %s
// RUN: not grep "unable to get symbolic solution" %t1.err
//
// With --write-kill-tests, the test of the state is a kill test of type solver
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --max-solver-time=1s --write-kill-tests %t.bc 2> %t2.err
// RUN: FileCheck --check-prefix=CHECK-KILL --input-file=%t2.err %s
// RUN: ls %t.klee-out-2 | grep "test000002.ktest"
// RUN: ls %t.klee-out-2 | grep "test000002.solver.err"
//
// The same with --exit-on-error-type that does not stop on a solver error
// RUN: rm -rf %t.klee-out-3
// RUN: %klee --output-dir=%t.klee-out-3 --max-solver-time=1s --exit-on-error-type=Assert --write-incomplete-tests %t.bc 2> %t3.err
// RUN: ls %t.klee-out-3 | grep "test000001.solver.err"
// RUN: ls %t.klee-out-3 | grep "test000001.ktest"
// CHECK-ERR: KLEE: ERROR: {{.*}}Query timed out (fork)
// CHECK-KILL: KLEE: WARNING: Wrote kill test: {{.*}}.ktest (type: solver, reason: Query timed out (fork).)
// CHECK-SUMMARY: KLEE: done: completed paths = 0
// CHECK-SUMMARY: KLEE: done: partially completed paths = 2
// CHECK-TEST: object 0: name: 'x'
#include "klee/klee.h"
#include <stdio.h>

int main() {
  long long int x, y = 102*75678 + 78, i = 101;

  klee_make_symbolic(&x, sizeof(x), "x");

  if (x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x + (x*x % (x+12)) == y*y*y*y*y*y*y*y*y*y*y*y*y*y*y*y % i)
    printf("Yes\n");
  else printf("No\n");

  return 0;
}
