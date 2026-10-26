// Without the option, there are no marker files
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out-0
// RUN: %klee --output-dir=%t.klee-out-0 --search=dfs %t.bc 2> %t0.err
// RUN: ls %t.klee-out-0 | FileCheck --check-prefix=CHECK-NONE --implicit-check-not=newcov %s
// CHECK-NONE: test000001.ktest

// With the option, only some of the tests (exits and the states dumped on
// halt) are marked: some cover new code, but not all of them
// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --search=dfs --mark-tests-covering-new --max-instructions=60 --dump-states-on-halt %t.bc 2> %t1.err
// RUN: ls %t.klee-out-1 | grep -c "\.newcov$" | FileCheck --check-prefix=CHECK-SOME %s
// CHECK-SOME: {{[1-9][0-9]*$}}
// RUN: ls %t.klee-out-1 | grep "\.early$" | FileCheck --check-prefix=CHECK-EARLY %s
// CHECK-EARLY: .early

// All the paths run to completion: every marker belongs to a test case, the
// first test (which covers new code) is marked
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --search=dfs --mark-tests-covering-new %t.bc 2> %t2.err
// RUN: ls %t.klee-out-2 | grep -c "\.ktest$" | FileCheck --check-prefix=CHECK-ALL %s
// CHECK-ALL: 8
// RUN: ls %t.klee-out-2 | grep "\.newcov$" | FileCheck --check-prefix=CHECK-FIRST %s
// CHECK-FIRST: test000001.newcov
// RUN: ls %t.klee-out-2 | grep -c "\.newcov$" | FileCheck --check-prefix=CHECK-FEW %s
// CHECK-FEW: {{[1-7]$}}

#include "klee/klee.h"

int main(void) {
  char c[3];
  klee_make_symbolic(c, sizeof(c), "c");
  int n = 0;
  for (int i = 0; i < 3; ++i)
    if (c[i])
      ++n;
  return n;
}
