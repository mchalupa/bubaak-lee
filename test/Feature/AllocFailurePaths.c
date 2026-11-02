// A failed allocation is the entry 4294967295:0 of the path over the marked
// branches, a successful one is not on the path (the paths without failures
// are those of the runs that do not model them).
// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// (the marked branches: 0 is `!p` (the direction 0: p is not null), 1 is
// `*p > 0`)

// Every test has its path
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail --write-unfinished-paths %t.bc > %t.log 2> %t.err
// RUN: %paths show %t.klee-out/*.path | sort > %t.paths
// RUN: printf "0:0 1:0\n0:0 1:1\n4294967295:0 0:1\n" | diff - %t.paths

// A prefix path through the failure explores only below it
// RUN: rm -rf %t.klee-out && echo "f 4294967295:0" | %paths list > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --alloc-may-fail --prefix-paths=%t.prefixes %t.bc > %t.log 2> %t.err
// RUN: grep -qx "null" %t.log
// RUN: not grep -q "pos\|neg" %t.log
// RUN: grep -q "completed paths = 1" %t.err
// RUN: grep -qx "f" %t.klee-out/used-prefix-paths.txt

// A prefix path without the failure does not take it
// RUN: rm -rf %t.klee-out && echo "s 0:0" | %paths list > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --alloc-may-fail --prefix-paths=%t.prefixes %t.bc > %t.log 2> %t.err
// RUN: not grep -q "null" %t.log
// RUN: grep -qx "pos" %t.log
// RUN: grep -qx "neg" %t.log
// RUN: grep -q "completed paths = 2" %t.err

// A done path of a run without failures does not end the failed one
// RUN: rm -rf %t.klee-out && echo "x 0:0 1:0" | %paths list > %t.done
// RUN: %klee --output-dir=%t.klee-out --alloc-may-fail --done-paths=%t.done %t.bc > %t.log 2> %t.err
// RUN: grep -qx "null" %t.log
// RUN: grep -qx "neg" %t.log
// RUN: not grep -q "pos" %t.log
// RUN: grep -q "completed paths = 2" %t.err

// The paths of the dropped branches (forks inhibited) are written, KLEE
// continues them (the failure is the first fork)
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail --write-unfinished-paths --max-forks=0 %t.bc > %t.log 2> %t.err
// RUN: %paths show %t.klee-out/unfinished.paths > %t.unfinished
// RUN: awk '{ print "d" NR " " $0 }' %t.unfinished | %paths list > %t.prefixes
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 --alloc-may-fail --prefix-paths=%t.prefixes %t.bc > %t.log2 2> %t.err2
// RUN: cat %t.log %t.log2 | sort -u | wc -l | grep -qx 3
#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int a;
  klee_make_symbolic(&a, sizeof a, "a");
  int *p = malloc(sizeof(int));
  if (!p) {
    printf("null\n");
    return 0;
  }
  *p = a;
  if (*p > 0)
    printf("pos\n");
  else
    printf("neg\n");
  free(p);
  return 0;
}
