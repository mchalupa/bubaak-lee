// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc

// The path of a state with a concretized value is written once (the
// concretizations on the same path are not written again)
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths %t.bc > %t.log 2> %t.err
// RUN: %paths show %t.klee-out/dropped.paths > %t.dropped
// RUN: wc -l < %t.dropped | grep -qx 1
// RUN: grep -q "0:0$" %t.dropped
#include "klee/klee.h"
#include <stdio.h>

int main(void) {
  int a;
  klee_make_symbolic(&a, sizeof a, "a");
  if (a > 0) {
    // (KLEE concretizes the floating-point values)
    volatile double d = a * 0.5;
    volatile double e = a * 0.25;
  }
  return 0;
}
