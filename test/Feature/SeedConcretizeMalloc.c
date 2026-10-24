// RUN: %clang -emit-llvm -c -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=SeedGen %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest
// RUN: rm -rf %t.klee-out-200
// RUN: %klee --output-dir=%t.klee-out-200 --entry-point=SeedGen200 %t.bc

// The seeded state allocates the size of the seed, the other sizes are
// explored by an unseeded state.
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest %t.bc | FileCheck --check-prefix=CHECK-ALL %s
// RUN: FileCheck --check-prefix=CHECK-ALL-INFO %s < %t.klee-out-2/info

// Both seeds stay in the seeded state (the seed of size 200 is patched),
// no seeded state for the size 200.
// RUN: rm -rf %t.klee-out-4
// RUN: %klee --output-dir=%t.klee-out-4 --seed-file %t.klee-out/test000001.ktest --seed-file %t.klee-out-200/test000001.ktest %t.bc | FileCheck --check-prefix=CHECK-ALL %s
// RUN: FileCheck --check-prefix=CHECK-ALL-INFO %s < %t.klee-out-4/info

// Only the size of the seed with --only-replay-seeds.
// RUN: rm -rf %t.klee-out-3
// RUN: %klee --output-dir=%t.klee-out-3 --only-replay-seeds --seed-file %t.klee-out/test000001.ktest %t.bc | FileCheck --allow-empty %s
// RUN: FileCheck --check-prefix=CHECK-SEED-INFO %s < %t.klee-out-3/info
// RUN: %klee-stats --print-columns 'SolverQueries' --table-format=csv %t.klee-out-3 | FileCheck --check-prefix=CHECK-STATS %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void SeedGen() {
  size_t x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == 100);
}

void SeedGen200() {
  size_t x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == 200);
}

int main(int argc, char **argv) {
  size_t s;
  klee_make_symbolic(&s, sizeof(s), "size");
  char *p = (char *)malloc(s);
  if (!p)
    return 0;

  if (s != 100)
    printf("Error\n");
    // CHECK-ALL: Error
    // CHECK-NOT: Error

  // CHECK-ALL-INFO: KLEE: done: completed paths = 2
  // CHECK-SEED-INFO: KLEE: done: completed paths = 1

  // CHECK-STATS: 2
  // In the segment model, executeAlloc concretizes via the seed: the fork on
  // the size of the seed and the if(s!=100) branch check (resolved by the
  // seed, no extra forking) trigger a solver query each.
}
