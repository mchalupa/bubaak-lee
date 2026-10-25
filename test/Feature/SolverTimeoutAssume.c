// -max-solver-time limits also the query of klee_assume (and
// __VERIFIER_assume), the state is killed by the solver and has a test
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-solver-time=1s --write-incomplete-tests --max-instructions=1000 %t.bc 2> %t.err
// RUN: FileCheck --input-file=%t.err %s
// RUN: ls %t.klee-out | grep ".solver.err"
// CHECK: KLEE: ERROR: {{.*}}Query timed out (klee_assume)
// CHECK: KLEE: done: completed paths = 0
// CHECK: KLEE: done: partially completed paths = 2
#include "klee/klee.h"

int main() {
  long long x;
  klee_make_symbolic(&x, sizeof x, "x");
  klee_assume(x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x + (x*x % (x+12)) == 77777777777LL);
  return 0;
}
