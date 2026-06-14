// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --warnings-only-to-file=false %t1.bc 2>&1 | FileCheck %s
// REQUIRES: not-freebsd

// Tests that a symbolic write to an object larger than 4 GiB is rejected;
// the internal 32-bit offset representation cannot model it correctly.

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
  p[k] = 42;
  // CHECK: Symbolic writes to objects larger than 4 GiB are not allowed (object size: 4294967297 bytes)
  return 0;
}
