// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --warnings-only-to-file=false %t1.bc 2>&1 | FileCheck %s
// REQUIRES: not-freebsd

// Tests that a symbolic write to a huge (> 4 GiB) object succeeds.
// No physical memory is involved: KLEE models the write as an update node in
// its symbolic array, so the truncated sizeBound is not a problem for writes.
// (Symbolic reads from such objects are still rejected because they would
//  incorrectly fall back to the 1-byte concrete backing store.)

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>

int main() {
  size_t sz = (size_t)1 + ((size_t)1 << 32);  // 4294967297
  char *p = malloc(sz);
  // CHECK: WARNING ONCE: Large memory allocation (4294967297 bytes). KLEE may run out of memory.
  assert(p);

  unsigned k;
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 100);
  p[k] = 42;  // symbolic write — should succeed

  // CHECK-NOT: KLEE: ERROR
  // CHECK: KLEE: done:
  return 0;
}
