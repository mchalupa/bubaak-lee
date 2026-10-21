// REQUIRES: x86_64

// An empty inline assembly with an output that is not tied to an input has
// an unknown result: it becomes a nondet value, which over-approximates the
// value, so the errors on the paths that use it are not reported.

// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure %t.bc 2>&1 | FileCheck %s
// CHECK: KLEE: WARNING ONCE: over-approximation: the result of inline assembly is a nondet value

#include "klee/klee.h"
#include <assert.h>

int main() {
  int x;
  __asm__ __volatile__("" : "=r"(x));
  if (x == 3)
    // CHECK: ERROR: {{.*}}EmptyAsmUntiedOutput.c:[[@LINE+1]]: over-approximated path (the result of inline assembly), its error is not reported
    assert(0);
  // CHECK-NOT: ASSERTION FAIL
  return 0;
}
