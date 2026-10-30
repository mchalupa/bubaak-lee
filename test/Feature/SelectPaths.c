// RUN: %clang -O1 -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// RUN: grep -q "bbk.select" %t.marked.ll
// (the marked branch 0 `r == 3`, the marked select 1 `a > 0 ? 3 : 5`; the
// direction 0 is the true branch, the true value)

// With -cover-selects, a fork at a select is an entry of the path
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths --cover-selects %t.bc > %t.log 2> %t.err
// RUN: %paths show %t.klee-out/*.path | sort > %t.paths
// RUN: grep -qx "1:0 0:0" %t.paths
// RUN: grep -qx "1:1 0:1" %t.paths
// RUN: wc -l < %t.paths | grep -qx 2

// Without it, the select is not on the path (the branch forks)
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 --write-unfinished-paths %t.bc > %t.log2 2> %t.err2
// RUN: %paths show %t.klee-out-2/*.path | sort > %t.paths2
// RUN: grep -qx "0:0" %t.paths2
// RUN: grep -qx "0:1" %t.paths2
// RUN: wc -l < %t.paths2 | grep -qx 2

// On a prefix path through a side of the select, the select forks (only
// that side), also without -cover-selects
// RUN: printf "p1 1:0\n" | %paths list > %t.prefix-3
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --write-unfinished-paths --prefix-paths=%t.prefix-3 %t.bc > %t.log3 2> %t.err3
// RUN: %paths show %t.klee-out-3/*.path > %t.paths3
// RUN: grep -qx "1:0 0:0" %t.paths3
// RUN: wc -l < %t.paths3 | grep -qx 1
// RUN: grep -qx "p1" %t.klee-out-3/used-prefix-paths.txt

// On a prefix path without the select, the select does not fork (even if
// it is not covered)
// RUN: printf "p1 0:1\n" | %paths list > %t.prefix-4
// RUN: rm -rf %t.klee-out-4 && %klee --output-dir=%t.klee-out-4 --write-unfinished-paths --cover-selects --prefix-paths=%t.prefix-4 %t.bc > %t.log4 2> %t.err4
// RUN: %paths show %t.klee-out-4/*.path > %t.paths4
// RUN: grep -qx "0:1" %t.paths4
// RUN: wc -l < %t.paths4 | grep -qx 1
// RUN: grep -qx "p1" %t.klee-out-4/used-prefix-paths.txt
#include "klee/klee.h"
#include <stdio.h>

int main(void) {
  int a;
  klee_make_symbolic(&a, sizeof a, "a");
  // (a select at -O1; the volatile and the different calls keep the branch)
  volatile int r = a > 0 ? 3 : 5;
  if (r == 3)
    printf("positive\n");
  else
    printf("other %d\n", r);
  return 0;
}
