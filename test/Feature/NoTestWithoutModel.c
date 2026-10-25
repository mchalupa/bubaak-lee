// If the values of a test are not known (no model of the path constraints,
// no seed, no prefix of the constraints with a model), no test is written
// (the values are not invented), a warning says it, and no kill test is
// announced.
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
//
// The query for the model fails, the model of a prefix of the constraints is used
// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --debug-fail-test-model=1 --write-testcases --max-instructions=100 %t.bc 2> %t1.err
// RUN: FileCheck --check-prefix=CHECK-PREFIX --input-file=%t1.err %s
// RUN: ls %t.klee-out-1 | grep test000001.ktest
// RUN: ls %t.klee-out-1 | grep test000001.xml
// RUN: %ktest-tool %t.klee-out-1/test000001.ktest | FileCheck --check-prefix=CHECK-KTEST %s
// CHECK-PREFIX: KLEE: WARNING ONCE: unable to compute a model of the path constraints of a state
// CHECK-KTEST: object 0: int : {{[0-9]+}}
//
// No model at all: no test
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --debug-fail-test-model=2 --write-testcases --write-kill-tests --write-incomplete-tests --max-instructions=100 %t.bc 2> %t2.err
// RUN: FileCheck --check-prefix=CHECK-NONE --input-file=%t2.err %s
// RUN: ls %t.klee-out-2 | not grep ".ktest"
// RUN: ls %t.klee-out-2 | not grep ".xml"
// CHECK-NONE: KLEE: WARNING: unable to compute the values of the test of state {{[0-9]+}} {{.*}}no test written
// CHECK-NONE: KLEE: WARNING: unable to get the values of the test case
// CHECK-NONE: partially completed paths = 1
// CHECK-NONE: KLEE: done: generated tests = 0
#include "klee/klee.h"
int __VERIFIER_nondet_int(void);
int main(void) {
  int x = __VERIFIER_nondet_int();
  klee_assume(x > 4);
  while (1) { volatile int z = 0; z++; if (z > 1) break; }
  return 0;
}
