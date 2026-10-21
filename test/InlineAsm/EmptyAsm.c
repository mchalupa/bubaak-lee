// REQUIRES: x86_64

// An empty inline assembly computes nothing: an output that is tied to an
// input keeps the value of the input (the index of the input among the
// operands is not the digit of the constraint), and the asm is lowered away
// (no external call, the symbolic arguments are fine).

// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure --exit-on-error %t.bc 2>&1 | FileCheck %s
// CHECK-NOT: over-approximation
// CHECK-NOT: external call
// CHECK: KLEE: done: completed paths = 1

#include "klee/klee.h"
#include <assert.h>

int main() {
  long x = klee_int("x");
  long y = klee_int("y");
  long z = klee_int("z");
  long a = x, b = y;

  // "=r,0"
  __asm__ __volatile__("" : "+r"(a));
  assert(a == x);

  // several outputs (a struct result), "=r,=r,r,1,0": the input tied to
  // the first output is the third operand
  __asm__ __volatile__("" : "=r"(a), "=r"(b) : "r"(z), "1"(x), "0"(y));
  assert(a == y);
  assert(b == x);

  // a memory output and a memory barrier
  long m = z;
  __asm__ __volatile__("" : "+m"(z) : : "memory");
  assert(z == m);
  __asm__ __volatile__("" : : : "memory");

  return 0;
}
