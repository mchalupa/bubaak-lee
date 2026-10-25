// -max-dump-minimization-time limits the minimization of the tests of the
// states dumped on halt, after it the tests have the values of a single model
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out-0
// RUN: %klee --output-dir=%t.klee-out-0 --max-instructions=500 %t.bc 2> %t0.err
// RUN: %ktest-tool %t.klee-out-0/test000001.ktest %t.klee-out-0/test000002.ktest | FileCheck --check-prefix=CHECK-MIN %s
// CHECK-MIN: int : 1000001
//
// With a budget that is used up at once: every dumped state has a test
// (with the values of a model)
// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --max-instructions=500 --max-dump-minimization-time=1us %t.bc 2> %t1.err
// RUN: ls %t.klee-out-1 | grep test000001.ktest
// RUN: ls %t.klee-out-1 | grep test000002.ktest
// RUN: %ktest-tool %t.klee-out-1/test000001.ktest %t.klee-out-1/test000002.ktest | FileCheck --check-prefix=CHECK-MODEL %s
// CHECK-MODEL-NOT: int : 1000001
// CHECK-MODEL: int : 77778
//
// With enough time, the values are minimized
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --max-instructions=500 --max-dump-minimization-time=60s %t.bc 2> %t2.err
// RUN: %ktest-tool %t.klee-out-2/test000001.ktest %t.klee-out-2/test000002.ktest | FileCheck --check-prefix=CHECK-MIN %s
#include "klee/klee.h"
int __VERIFIER_nondet_int(void);
int main(void) {
  int x = __VERIFIER_nondet_int();
  int y = __VERIFIER_nondet_int();
  klee_assume((x > 4) & (y > 77777));
  if (x > 1000000) {
    volatile int z = 0;
    while (1) z++;
  }
  while (1) { volatile int z = 0; z++; if (z > 1) break; }
  return 0;
}
