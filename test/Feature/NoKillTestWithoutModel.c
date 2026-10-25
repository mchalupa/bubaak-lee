// A state killed by the solver whose test cannot be written is not announced
// as a kill test (no file F), but still counts as partially completed
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-solver-time=1s --debug-fail-test-model=2 --write-kill-tests --write-incomplete-tests %t.bc 2> %t.err
// RUN: FileCheck --input-file=%t.err %s
// RUN: not grep "Wrote kill test" %t.err
// RUN: ls %t.klee-out | grep "test000002.solver.err"
// RUN: ls %t.klee-out | not grep ".ktest"
// CHECK: unable to compute the values of the test of state {{[0-9]+}} {{.*}}no test written
// CHECK: Query timed out (fork)
// CHECK: partially completed paths = 2
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
