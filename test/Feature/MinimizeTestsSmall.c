// The nondet values of a test are small if the constraints allow it
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-test-minimization-time=60s %t.bc 2> %t.err
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck %s
// RUN: rm -rf %t.klee-out-1
// Even with 1us (no time for refining), the test is valid
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --max-test-minimization-time=1us %t.bc 2> %t2.err
// RUN: ls %t.klee-out-2 | grep test000001.ktest
// CHECK: object 0: int : 5
// CHECK: object 1: int : 3000
// CHECK: object 2: int : 0

#include "klee/klee.h"

int __VERIFIER_nondet_int(void);

int main(void) {
  int x = __VERIFIER_nondet_int();
  int y = __VERIFIER_nondet_int();
  int z = __VERIFIER_nondet_int();
  klee_assume(x > 4);
  klee_assume(y > 2999);
  klee_assume(z >= 0 & z < 1000);
  return 0;
}
