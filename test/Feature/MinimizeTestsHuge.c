// Only huge values satisfy the constraints: the test is still valid
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2> %t.err
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck %s
// CHECK: object 0: int : 2000000000
// CHECK: object 1: int : 7

#include "klee/klee.h"

int __VERIFIER_nondet_int(void);

int main(void) {
  int x = __VERIFIER_nondet_int();
  int y = __VERIFIER_nondet_int();
  klee_assume(x > 1999999999 & x < 2000000001);
  klee_assume(y == 7);
  return 0;
}
