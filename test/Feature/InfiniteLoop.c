// A path that reaches __bbk_infinite_loop (a loop marked by the pass
// bbk-infinite-loops of bubaak) of a kind of -kill-infinite-loops is
// complete, but it is not the end of the program: no leak is reported (the
// memory is lost only at the exit) and the path is not unfinished. Otherwise
// (another kind, no -kill-infinite-loops, the path called __bbk_async) the
// call does nothing and the path goes on to the exit (a leak).
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --kill-infinite-loops=idempotent --check-memcleanup --write-unfinished-paths %t.bc 2>&1 | FileCheck %s
// RUN: wc -c < %t.klee-out/unfinished.paths | FileCheck %s -check-prefix=CHECK-UNFINISHED
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --kill-infinite-loops=no-exit,idempotent --check-leaks %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-memcleanup %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-CONT --implicit-check-not="infinite loop"
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --kill-infinite-loops=no-exit --check-memcleanup %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-CONT --implicit-check-not="infinite loop"
// RUN: %clang %s -DASYNC -emit-llvm %O0opt -g -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --kill-infinite-loops=idempotent --check-memcleanup %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-CONT --implicit-check-not="infinite loop"

// CHECK: a path reached an infinite loop
// CHECK-NOT: memory not cleaned up
// CHECK-NOT: memory leak detected
// CHECK: KLEE: done: completed paths = 2
// CHECK-CONT: memory not cleaned up
// (only the header of the list of paths: no unfinished path)
// CHECK-UNFINISHED: {{^ *5$}}

#include "klee/klee.h"

#include <stdlib.h>

void __bbk_infinite_loop(int kind);
void __bbk_async(void);

int main(void) {
  char *p = malloc(4);
  int x;
  klee_make_symbolic(&x, sizeof x, "x");
#ifdef ASYNC
  __bbk_async();
#endif
  if (x) {
    __bbk_infinite_loop(1); // an idempotent iteration
    exit(0);
  }
  free(p);
  return 0;
}
