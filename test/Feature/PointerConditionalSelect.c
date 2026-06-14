// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc 2>&1 | FileCheck %s

// Tests pointer comparison after a symbolic condition selects between two
// concrete global pointers.  KLEE forks on the condition; in each branch the
// pointer is concrete so the comparison resolves without a solver query.

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>

int a = 10, b = 20;

int main() {
  int cond;
  klee_make_symbolic(&cond, sizeof(cond), "cond");

  int *p = cond ? &a : &b;

  if (p == &a) {
    assert(*p == 10);
    printf("a\n");
  } else if (p == &b) {
    assert(*p == 20);
    printf("b\n");
  } else {
    klee_assert(0);  // unreachable: p must be &a or &b
  }

  // CHECK-DAG: a
  // CHECK-DAG: b
  // CHECK-DAG: KLEE: done: generated tests = 2
  return 0;
}
