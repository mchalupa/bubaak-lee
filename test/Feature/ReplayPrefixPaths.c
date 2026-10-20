// RUN: %clang -emit-llvm -c -g %s -o %t.bc

// The paths record only the branches where both sides are feasible
// (`a > 10` is implied by `a > 20`). A run that halts (max-forks) writes
// the paths of all states.
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths --max-forks=1 %t.bc > %t.log 2> %t.err
// RUN: grep -q "^1[01]#" %t.klee-out/unfinished-paths.txt
// RUN: awk '{ print "u" NR " " $0 }' %t.klee-out/unfinished-paths.txt > %t.prefixes

// The states replay the prefix paths and continue from their ends
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 --prefix-paths=%t.prefixes --write-unfinished-paths %t.bc > %t.log2 2> %t.err2
// RUN: wc -l < %t.klee-out-2/used-prefix-paths.txt | grep -qx 2
// RUN: cat %t.log %t.log2 | sort -u | wc -l | grep -qx 4
// RUN: not grep -q "misaligned" %t.err2

// A state that did not reach the end of a prefix path does not write its
// path (the prefix paths stay unfinished): with the prefix paths 10 and
// 11, the state cannot fork at `b > 0`, the dropped side is not written
// RUN: printf "p1 10\np2 11\n" > %t.prefixes-3
// RUN: rm -rf %t.klee-out-6 && %klee --output-dir=%t.klee-out-6 --prefix-paths=%t.prefixes-3 --write-unfinished-paths --max-forks=0 %t.bc > %t.log6 2> %t.err6
// RUN: wc -l < %t.klee-out-6/used-prefix-paths.txt | grep -qx 1
// RUN: wc -l < %t.klee-out-6/unfinished-paths.txt | grep -qx 0

// A killed state (a failed external call) writes its path to
// killed-paths.txt; only seeded states continue from it
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --write-unfinished-paths --external-calls=all %t.bc kill > %t.log3 2> %t.err3
// RUN: grep -q "(killed path written)" %t.err3
// RUN: grep -q "^0#" %t.klee-out-3/killed-paths.txt
// RUN: awk '{ print "k" NR " " $0 }' %t.klee-out-3/killed-paths.txt > %t.killed
// RUN: rm -rf %t.klee-out-4 && %klee --output-dir=%t.klee-out-4 --seeded-prefix-paths=%t.killed --write-unfinished-paths %t.bc kill > %t.log4 2> %t.err4
// RUN: wc -l < %t.klee-out-4/used-prefix-paths.txt | grep -qx 0
// RUN: not grep -q "misaligned" %t.err4
// A seeded state continues from it (and is killed again)
// RUN: rm -rf %t.klee-out-5 && %klee --output-dir=%t.klee-out-5 --seeded-prefix-paths=%t.killed --write-unfinished-paths --seed-file=%t.klee-out-3/test000001.ktest %t.bc kill > %t.log5 2> %t.err5
// RUN: grep -qx "k1" %t.klee-out-5/used-prefix-paths.txt
// RUN: grep -q "^0#" %t.klee-out-5/killed-paths.txt
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
