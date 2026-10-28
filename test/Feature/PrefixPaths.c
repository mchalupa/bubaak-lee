// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// (the marked branches: 0 is `a > 0`, 1 and 2 are `b > 0` below it; the
// direction 0 is the true branch)

// Only the paths below a prefix path are explored (0:0 is a > 0)
// RUN: rm -rf %t.klee-out && echo "p1 0:0" | %paths list > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --prefix-paths=%t.prefixes %t.bc > %t.log 2> %t.err
// RUN: grep -q "pos pos" %t.log
// RUN: grep -q "pos neg" %t.log
// RUN: not grep -q "^neg" %t.log
// RUN: grep -q "completed paths = 2" %t.err
// RUN: grep -q "partially completed paths = 0" %t.err
// RUN: grep -qx "p1" %t.klee-out/used-prefix-paths.txt

// A longer prefix path (a > 0, b <= 0) and a path that cannot be followed
// (the branch 1 is not on the path after a <= 0)
// RUN: rm -rf %t.klee-out && printf "p2 0:0 1:1\np3 0:1 1:0\n" | %paths list > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --prefix-paths=%t.prefixes %t.bc > %t.log 2> %t.err
// RUN: grep -q "pos neg" %t.log
// RUN: not grep -q "pos pos" %t.log
// RUN: grep -q "completed paths = 1" %t.err
// RUN: grep -qx "p2" %t.klee-out/used-prefix-paths.txt
// RUN: not grep -q "p3" %t.klee-out/used-prefix-paths.txt

// Every test has its path
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths %t.bc > %t.log 2> %t.err
// RUN: %paths show %t.klee-out/*.path | sort > %t.paths
// RUN: printf "0:0 1:0\n0:0 1:1\n0:1 2:0\n0:1 2:1\n" | diff - %t.paths

// The paths of the dropped branches are written, KLEE continues them
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths --max-forks=1 %t.bc > %t.log 2> %t.err
// RUN: grep -q "completed paths = 2" %t.err
// RUN: %paths show %t.klee-out/unfinished.paths > %t.unfinished
// RUN: grep -c "^0:[01] [12]:[01]$" %t.unfinished | grep -qx 2
// RUN: awk '{ print "d" NR " " $0 }' %t.unfinished | %paths list > %t.prefixes
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 --prefix-paths=%t.prefixes %t.bc > %t.log2 2> %t.err2
// RUN: grep -q "completed paths = 2" %t.err2
// RUN: cat %t.log %t.log2 | sort -u | wc -l | grep -qx 4
// RUN: wc -l < %t.klee-out-2/used-prefix-paths.txt | grep -qx 2

// A path that does not start with the first marked branch is not followed
// RUN: echo "x 1:0" | %paths list > %t.bad-prefixes
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --prefix-paths=%t.bad-prefixes %t.bc > %t.log3 2> %t.err3
// RUN: grep -q "completed paths = 0" %t.err3
// RUN: wc -l < %t.klee-out-3/used-prefix-paths.txt | grep -qx 0
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
