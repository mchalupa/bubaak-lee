// This test checks that under using the over-approximate external call policy, the symbolic arguments are left unconstrained by the external call

// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=over-approx %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  printf("%d\n", x);
  // CHECK: external call with symbolic argument: printf
  // In the fork's over-approx policy, states with symbolic pointer args are
  // terminated, so the conditional branches below are not reached.
  if (x > 0) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
}
