// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc 2>&1 | FileCheck %s

// Tests that KLEE correctly branches on a symbolic index into a concrete array,
// exploring one path per reachable element.

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>

int arr[4] = {10, 20, 30, 40};

int main() {
  unsigned i;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_assume(i < 4);

  int val = arr[i];
  assert(val == 10 || val == 20 || val == 30 || val == 40);

  if (val == 10) printf("ten\n");
  else if (val == 20) printf("twenty\n");
  else if (val == 30) printf("thirty\n");
  else printf("forty\n");

  // CHECK-DAG: ten
  // CHECK-DAG: twenty
  // CHECK-DAG: thirty
  // CHECK-DAG: forty
  // CHECK-DAG: KLEE: done: generated tests = 4
  return 0;
}
