// The nondet values are minimized
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-test-minimization-time=60s %t.bc 2> %t1.err
// RUN: not grep "minimizing the values" %t1.err
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck --check-prefix=CHECK-MIN %s
// CHECK-MIN: object 0: int : 42
// CHECK-MIN: object 1: int : 101

// Without time for the minimization, the values are those of a model
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --max-test-minimization-time=1us %t.bc 2> %t2.err
// RUN: FileCheck --check-prefix=CHECK-WARN --input-file=%t2.err %s
// RUN: %ktest-tool %t.klee-out-2/test000001.ktest | FileCheck --check-prefix=CHECK-MODEL %s
// CHECK-WARN: KLEE: WARNING ONCE: minimizing the values of a test case took too long, using the values of a model
// CHECK-MODEL: object 0: int : 42
// CHECK-MODEL: object 1: int : {{10[12]}}

#include "klee/klee.h"

int __VERIFIER_nondet_int(void);

int main(void) {
  int x = __VERIFIER_nondet_int();
  int y = __VERIFIER_nondet_int();
  klee_assume((x == 42) & (y > 100) & (y < 103));
  return 0;
}
