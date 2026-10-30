// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// (the marked branches: 0 is `a > 0`, 1 and 2 are `b > 0` below it; the
// direction 0 is the true branch)

// The done paths end silently (no test), the other paths are explored
// RUN: rm -rf %t.klee-out && printf "x 0:0 1:0\nx 0:1 2:1\n" | %paths list > %t.done
// RUN: %klee --output-dir=%t.klee-out --done-paths=%t.done %t.bc > %t.log 2> %t.err
// RUN: grep -q "pos neg" %t.log
// RUN: grep -q "neg pos" %t.log
// RUN: not grep -q "pos pos" %t.log
// RUN: not grep -q "neg neg" %t.log
// RUN: grep -q "completed paths = 2" %t.err
// RUN: ls %t.klee-out/*.ktest | wc -l | grep -qx 2

// With prefix paths: only below them, the done paths below them end
// RUN: rm -rf %t.klee-out && echo "p1 0:0" | %paths list > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --prefix-paths=%t.prefixes --done-paths=%t.done %t.bc > %t.log 2> %t.err
// RUN: grep -q "pos neg" %t.log
// RUN: not grep -q "pos pos" %t.log
// RUN: not grep -q "^neg" %t.log
// RUN: grep -q "completed paths = 1" %t.err
// RUN: grep -qx "p1" %t.klee-out/used-prefix-paths.txt

// A done path that another done path continues does not end the state
// RUN: rm -rf %t.klee-out && printf "x 0:0\ny 0:0 1:0\n" | %paths list > %t.done2
// RUN: %klee --output-dir=%t.klee-out --done-paths=%t.done2 %t.bc > %t.log 2> %t.err
// RUN: grep -q "completed paths = 3" %t.err
#include "klee/klee.h"
#include <stdio.h>

int main(void) {
  int a, b;
  klee_make_symbolic(&a, sizeof a, "a");
  klee_make_symbolic(&b, sizeof b, "b");
  if (a > 0) {
    if (b > 0)
      printf("pos pos\n");
    else
      printf("pos neg\n");
  } else {
    if (b > 0)
      printf("neg pos\n");
    else
      printf("neg neg\n");
  }
  return 0;
}
