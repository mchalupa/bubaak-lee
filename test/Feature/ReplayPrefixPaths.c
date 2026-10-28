// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// (the marked branches: 0 `argc > 1`, 1 `argv[1][0] == 'k'`, 2 `a > 0`,
// 3 `a > 20`, 4 `a > 10`, 5 and 6 `b > 0`; the direction 0 is the true
// branch)

// The paths have all marked branches, also those with a concrete condition
// (`argc > 1`) and those implied by the path (`a > 10`). A run that halts
// (max-forks) writes the paths of all states.
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths --max-forks=1 %t.bc > %t.log 2> %t.err
// RUN: %paths show %t.klee-out/unfinished.paths > %t.unfinished
// RUN: grep -q "^0:1 3:0 4:0 5:[01]$" %t.unfinished
// RUN: grep -q "^0:1 3:1 6:[01]$" %t.unfinished
// RUN: awk '{ print "u" NR " " $0 }' %t.unfinished | %paths list > %t.prefixes

// The states replay the prefix paths and continue from their ends
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 --prefix-paths=%t.prefixes --write-unfinished-paths %t.bc > %t.log2 2> %t.err2
// RUN: wc -l < %t.klee-out-2/used-prefix-paths.txt | grep -qx 2
// RUN: cat %t.log %t.log2 | sort -u | wc -l | grep -qx 4

// A state that did not reach the end of a prefix path does not write its
// path (the prefix paths stay unfinished): with the two prefix paths of
// `a > 20`, the state cannot fork at `b > 0`, the dropped side is not
// written
// RUN: printf "p1 0:1 3:0 4:0 5:0\np2 0:1 3:0 4:0 5:1\n" | %paths list > %t.prefixes-3
// RUN: rm -rf %t.klee-out-6 && %klee --output-dir=%t.klee-out-6 --prefix-paths=%t.prefixes-3 --write-unfinished-paths --max-forks=0 %t.bc > %t.log6 2> %t.err6
// RUN: wc -l < %t.klee-out-6/used-prefix-paths.txt | grep -qx 1
// RUN: %paths show %t.klee-out-6/unfinished.paths | wc -l | grep -qx 0

// A killed state (a failed external call) writes its path to killed.paths;
// only seeded states continue from it
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --write-unfinished-paths --external-calls=all %t.bc kill > %t.log3 2> %t.err3
// RUN: grep -q "(killed path written)" %t.err3
// RUN: %paths show %t.klee-out-3/killed.paths > %t.killed-paths
// RUN: grep -qx "0:0 1:0 2:1" %t.killed-paths
// RUN: awk '{ print "k" NR " " $0 }' %t.killed-paths | %paths list > %t.killed
// RUN: rm -rf %t.klee-out-4 && %klee --output-dir=%t.klee-out-4 --seeded-prefix-paths=%t.killed --write-unfinished-paths %t.bc kill > %t.log4 2> %t.err4
// RUN: wc -l < %t.klee-out-4/used-prefix-paths.txt | grep -qx 0
// A seeded state continues from it (and is killed again)
// RUN: rm -rf %t.klee-out-5 && %klee --output-dir=%t.klee-out-5 --seeded-prefix-paths=%t.killed --write-unfinished-paths --seed-file=%t.klee-out-3/test000001.ktest %t.bc kill > %t.log5 2> %t.err5
// RUN: grep -qx "k1" %t.klee-out-5/used-prefix-paths.txt
// RUN: %paths show %t.klee-out-5/killed.paths | grep -qx "0:0 1:0 2:1"
#include "klee/klee.h"
#include <stdio.h>

// not defined anywhere: the external call fails
extern int undefined_function(int);

int main(int argc, char **argv) {
  int a, b;
  klee_make_symbolic(&a, sizeof a, "a");
  klee_make_symbolic(&b, sizeof b, "b");
  if (argc > 1 && argv[1][0] == 'k') {
    if (a > 0)
      printf("pos\n");
    else
      return undefined_function(a);
    return 0;
  }
  if (a > 20) {
    if (a > 10) {
      if (b > 0)
        printf("big pos\n");
      else
        printf("big neg\n");
    }
  } else {
    if (b > 0)
      printf("small pos\n");
    else
      printf("small neg\n");
  }
  return 0;
}
