// The memory cap applies while seeding
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc initial
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --max-memory=10 --allow-seed-extension --seed-file %t.klee-out/test000001.ktest %t.bc 2>&1 | FileCheck %s
// CHECK: over memory cap
#include "klee/klee.h"
#include <stdlib.h>
#include <string.h>

#define N 4096

static char *objects[N];

int main(int argc, char **argv) {
  int n;
  klee_make_symbolic(&n, sizeof n, "n");
  if (argc == 2 && strcmp(argv[1], "initial") == 0) {
    klee_assume(n == 7);
    return 0;
  }

  if (n == 7) {
    // symbolic objects take memory proportional to their size
    for (int i = 0; i < N; ++i) {
      objects[i] = malloc(1 << 15);
      klee_make_symbolic(objects[i], 1 << 15, "o");
    }
  }
  return 0;
}
