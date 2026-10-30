// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// (the marked branches: 0 `argc > 1`, 1 the comparison, 2 the loop (four
// iterations in each call of mix); the direction 0 is the true branch)

// A state whose query timed out writes its path to solver.paths
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --write-unfinished-paths --max-solver-time=100ms %t.bc > %t.log 2> %t.err
// RUN: grep -q "(solver path written)" %t.err
// RUN: %paths show %t.klee-out/solver.paths > %t.solver-paths
// RUN: grep -qx "0:1 2:0 2:0 2:0 2:0 2:1 2:0 2:0 2:0 2:0 2:1" %t.solver-paths

// The seed (the value, given by the run with an argument)
// RUN: rm -rf %t.klee-out-2 && %klee --output-dir=%t.klee-out-2 %t.bc seed > %t.log2 2> %t.err2
// RUN: grep -q "^found$" %t.log2

// A seeded state passes the branch of the failed query on a prefix through
// its side without the query
// RUN: printf "x1 0:1 2:0 2:0 2:0 2:0 2:1 2:0 2:0 2:0 2:0 2:1 1:0\n" | %paths list > %t.extended
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --write-unfinished-paths --max-solver-time=100ms --seed-file=%t.klee-out-2/test000001.ktest --allow-seed-extension --allow-seed-truncation --seeded-prefix-paths=%t.extended %t.bc > %t.log3 2> %t.err3
// RUN: not grep -q "Query timed out" %t.err3
// RUN: wc -l < %t.klee-out-3/used-prefix-paths.txt | grep -qx 1
// RUN: grep -q "^found$" %t.log3
#include "klee/klee.h"
#include <stdio.h>

static unsigned long long mix(unsigned long long x) {
  for (int i = 0; i < 4; ++i)
    x = (x * x + 0x9e3779b97f4a7c15ULL) ^ (x >> 29);
  return x;
}

int main(int argc, char **argv) {
  unsigned long long a;
  klee_make_symbolic(&a, sizeof a, "a");
  if (argc > 1)
    klee_assume(a == 12345);
  // (inverting the squarings, the query times out)
  if (mix(a) == mix(12345))
    printf("found\n");
  return 0;
}
