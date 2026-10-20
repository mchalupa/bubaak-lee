// RUN: %clang -emit-llvm -c -g %s -o %t.bc

// Only the paths below a prefix path are explored (1: a > 0)
// RUN: rm -rf %t.klee-out && echo "p1 1" > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --prefix-paths=%t.prefixes %t.bc > %t.log 2> %t.err
// RUN: grep -q "pos pos" %t.log
// RUN: grep -q "pos neg" %t.log
// RUN: not grep -q "^neg" %t.log
// RUN: grep -q "completed paths = 2" %t.err
// RUN: grep -q "partially completed paths = 0" %t.err
// RUN: grep -qx "p1" %t.klee-out/used-prefix-paths.txt

// A longer prefix path (10: a > 0, b <= 0) and a path that cannot be
// followed (2)
// RUN: rm -rf %t.klee-out && printf "p2 10\np3 2\n" > %t.prefixes
// RUN: %klee --output-dir=%t.klee-out --prefix-paths=%t.prefixes %t.bc > %t.log 2> %t.err
// RUN: grep -q "pos neg" %t.log
// RUN: not grep -q "pos pos" %t.log
// RUN: grep -q "completed paths = 1" %t.err
// RUN: grep -qx "p2" %t.klee-out/used-prefix-paths.txt
// RUN: not grep -q "p3" %t.klee-out/used-prefix-paths.txt

// The paths of the dropped branches are written, KLEE continues them
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths --max-forks=1 %t.bc > %t.log 2> %t.err
// RUN: grep -q "completed paths = 2" %t.err
// RUN: grep -c "^[01][01]#[0-9a-f]*$" %t.klee-out/unfinished-paths.txt | grep -qx 2
// RUN: awk '{ print "d" NR " " $0 }' %t.klee-out/unfinished-paths.txt > %t.prefixes
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 --prefix-paths=%t.prefixes %t.bc > %t.log2 2> %t.err2
// RUN: grep -q "completed paths = 2" %t.err2
// RUN: cat %t.log %t.log2 | sort -u | wc -l | grep -qx 4
// RUN: wc -l < %t.klee-out-2/used-prefix-paths.txt | grep -qx 2

// A path whose hash does not match is not replayed (misaligned replay)
// RUN: sed 's/#.*/#0000000000000001/' %t.prefixes > %t.bad-prefixes
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --prefix-paths=%t.bad-prefixes %t.bc > %t.log3 2> %t.err3
// RUN: grep -q "misaligned replay" %t.err3
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
