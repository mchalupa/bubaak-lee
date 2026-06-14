// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --warnings-only-to-file=false --exit-on-error %t1.bc 2>&1 | FileCheck %s

// REQUIRES: not-freebsd

// Tests that a symbolic read from a huge (>4 GiB) concrete object works
// correctly.  Bytes not covered by the concrete store fall back to
// initialValue (0), so p[k] is either 42 (k==1) or 0 (all other k<100).
// The condition p[k]==3 is therefore unsatisfiable and KLEE never forks on it.

#include <assert.h>
#include <stdlib.h>

#include "klee/klee.h"

int main() {
  size_t sz = (size_t) 1 + ((size_t) 1 << 32);
  char *p = malloc(sz);
  // CHECK: WARNING ONCE: Large memory allocation (4294967297 bytes). KLEE may run out of memory.
  assert(p);
  p[1] = 42;

  unsigned k;
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 100);
  if (p[k] == 3)
    klee_assert(0);  // unreachable: p[k] is 0 or 42, never 3

  // CHECK-NOT: KLEE: ERROR
  // CHECK: KLEE: done:
}
