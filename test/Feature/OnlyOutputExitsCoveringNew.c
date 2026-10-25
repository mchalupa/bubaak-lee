// All the 8 completed paths get a test case
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc 2> %t.err
// RUN: ls %t.klee-out | grep -c "\.ktest$" | FileCheck --check-prefix=CHECK-ALL %s
// CHECK-ALL: 8

// With -only-output-exits-covering-new, only the paths that cover new code
// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --search=dfs --only-output-exits-covering-new %t.bc 2> %t1.err
// RUN: ls %t.klee-out-1 | grep -c "\.ktest$" | FileCheck --check-prefix=CHECK-NEW %s
// CHECK-NEW: {{[1-4]$}}

// The states dumped on halt get a test case even if they cover nothing new
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --search=dfs --only-output-exits-covering-new --max-instructions=60 --dump-states-on-halt %t.bc 2> %t2.err
// RUN: ls %t.klee-out-2 | grep -c "\.early$" | FileCheck --check-prefix=CHECK-EARLY %s
// CHECK-EARLY: {{[1-9]}}

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
