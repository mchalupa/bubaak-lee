// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// Casting a pointer to an integer (ptrtoint) and back (inttoptr) must preserve
// the pointer's identity in the segment-based memory model: the reconstructed
// pointer compares equal to the original and aliases the same object. This is
// the representation cross-object pointer arithmetic relies on.

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>

int g = 7;

int main() {
  int x = 5;
  int *p = &x;

  uintptr_t a = (uintptr_t)p;
  int *q = (int *)a;

  assert(q == p);     // round-trip preserves identity
  *q = 99;
  assert(x == 99);    // and still aliases the original object

  // Same for a global.
  int *gp = (int *)(uintptr_t)&g;
  assert(*gp == 7);

  return 0;
}
