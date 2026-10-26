// A path covers new code (a == 1) and then forks on a branch whose both
// directions are covered already: only one of the two states would keep the
// flag that the path covered new code. That state ends without a test
// (klee_silent_exit), so the other one must get a test of the new code.
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs --only-output-exits-covering-new %t.bc 2> %t.err
// RUN: %ktest-tool %t.klee-out/*.ktest | FileCheck %s
// CHECK: name: 'a'
// CHECK: int : 1

#include "klee/klee.h"

static int marked;

// a call (not inlined) keeps the branches apart
__attribute__((noinline)) static void mark(void) { ++marked; }

int main(void) {
  char a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  if (a == 1)
    mark();
  if (b) {
    if (marked)
      klee_silent_exit(0);
  }
  return 0;
}
