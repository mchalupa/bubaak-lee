// RUN: %clang -S -emit-llvm -g %s -o %t.ll
// RUN: %paths mark %t.ll > %t.marked.ll
// RUN: %llvmas %t.marked.ll -o %t.bc
// (the marked branches: 0 `argc == 2`, 1 `argc == 3`, 2 the comparison, 3
// the loop (four iterations in each call of mix); the direction 0 is the
// true branch)

// The seeds (the values, given by the runs with one and two arguments)
// RUN: rm -rf %t.klee-out-1 %t.klee-out-2 %t.seeds && mkdir %t.seeds
// RUN: %klee --output-dir=%t.klee-out-1 %t.bc x > %t.log1 2> %t.err1
// RUN: %klee --output-dir=%t.klee-out-2 %t.bc x y > %t.log2 2> %t.err2
// RUN: cp %t.klee-out-1/test000001.ktest %t.seeds/a.ktest

// A seeded state on prefixes through both sides of the comparison: with a
// seed only through one side, the query is needed (and times out)
// RUN: printf "p1 0:1 1:1 3:0 3:0 3:0 3:0 3:1 3:0 3:0 3:0 3:0 3:1 2:0\np2 0:1 1:1 3:0 3:0 3:0 3:0 3:1 3:0 3:0 3:0 3:0 3:1 2:1\n" | %paths list > %t.prefix
// RUN: rm -rf %t.klee-out-3 && %klee --output-dir=%t.klee-out-3 --write-unfinished-paths --max-solver-time=100ms --seed-dir=%t.seeds --allow-seed-extension --allow-seed-truncation --prefix-paths=%t.prefix %t.bc > %t.log3 2> %t.err3
// RUN: grep -q "Query timed out" %t.err3

// With seeds through both sides, both sides are feasible without the query
// RUN: cp %t.klee-out-2/test000001.ktest %t.seeds/b.ktest
// RUN: rm -rf %t.klee-out-4 && %klee --output-dir=%t.klee-out-4 --write-unfinished-paths --max-solver-time=100ms --seed-dir=%t.seeds --allow-seed-extension --allow-seed-truncation --prefix-paths=%t.prefix %t.bc > %t.log4 2> %t.err4
// RUN: not grep -q "Query timed out" %t.err4
// RUN: wc -l < %t.klee-out-4/used-prefix-paths.txt | grep -qx 2
// RUN: grep -q "^found$" %t.log4
// RUN: grep -q "^other$" %t.log4
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
  if (argc == 2)
    klee_assume(a == 12345);
  if (argc == 3)
    klee_assume(a == 1);
  // (inverting the squarings, the query times out)
  if (mix(a) == mix(12345))
    printf("found\n");
  else
    printf("other\n");
  return 0;
}
